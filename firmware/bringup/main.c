/*
 * Copyright (c) 2021, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * ER2024 Step 7: One-lap line PID with encoder-distance stop.
 *
 * Hardware:
 *   PWM:  TIMA0 CCP0=PA8 (HW-A/PWMA/AO -> physical right wheel),
 *         CCP1=PA9 (HW-B/PWMB/BO -> physical left wheel),
 *         4 kHz, timerCount=1000
 *   Dir:  AIN1=PA16, AIN2=PA21, BIN1=PA22, BIN2=PA23
 *   STBY: PB20 (active high)
 *   Enc:  HW-A side: ENC1A=PB17(irq), ENC1B=PB18
 *         HW-B side: ENC2A=PB24(irq), ENC2B=PB25
 *
 * Control:
 *   UART command 'g' starts a one-lap run from A.
 *   UART command 'G' keeps the old continuous line-following test mode.
 *   Sensor raw bits -> weighted line error -> PD steering correction.
 *   Logical left/right wheel PWM are transformed through orientation flags,
 *   so chassis front/left mistakes can be fixed without rewiring.
 *
 * Serial output @ 9600-8N1:
 *   mode=RUN line=0bXXXXXXXX st=OK err=%d corr=%d base=%d L=%d R=%d encL=%d encR=%d odom=%d
 */

#include "ti_msp_dl_config.h"

#include <stdint.h>

#ifndef DL_UART_Main_readData
#define DL_UART_Main_readData DL_UART_Main_receiveData
#endif

/* ---- timing ---- */
#define STATUS_PRINT_PERIOD_MS    (500U)
#define STATUS_PRINT_AUTO_PERIOD_MS (1000U)
#define LINE_PID_PERIOD_MS        (20U)    /* 50 Hz first floor-test loop */
#define LED_TOGGLE_PERIOD_MS      (500U)
#define LAP_DONE_LED_PERIOD_MS    (100U)
#define LINE_SENSOR_SETTLE_US     (50U)
#define LINE_SENSOR_CHANNEL_COUNT (8U)
#define STEP7_LOST_GRACE_MS       (300U)
#define STEP7_LOST_GRACE_TICKS    (STEP7_LOST_GRACE_MS / LINE_PID_PERIOD_MS)

#define PWM_MAX                   (1000)   /* timerCount */

#define STEP6_BASE_SPEED_DEFAULT  (180)    /* 18% duty; raise by UART if stuck */
#define STEP6_BASE_SPEED_MIN      (120)
#define STEP6_BASE_SPEED_MAX      (420)
#define STEP6_STEER_MAX           (240)
#define STEP6_PID_SCALE           (100)
#define STEP6_KP_DEFAULT          (55)
#define STEP6_KI_DEFAULT          (0)
#define STEP6_KD_DEFAULT          (35)
#define STEP6_INTEGRAL_LIMIT      (400)

#define STEP7_LAP_TARGET_TICKS_DEFAULT (26000)
#define STEP7_LAP_TARGET_TICKS_MIN     (1000)
#define STEP7_LAP_TARGET_TICKS_MAX     (120000)
#define STEP7_LAP_TARGET_STEP          (1000)
#define STEP7_GAP_BALANCE_POS_KP_NUM   (10)
#define STEP7_GAP_BALANCE_POS_KP_DEN   (100)
#define STEP7_GAP_BALANCE_SPEED_KP_NUM (250)
#define STEP7_GAP_BALANCE_SPEED_KP_DEN (100)
#define STEP7_GAP_BALANCE_MAX          (150)
#define STEP7_GAP_BALANCE_BIAS_DEFAULT (15)
#define STEP7_GAP_BALANCE_BIAS_MIN     (0)
#define STEP7_GAP_BALANCE_BIAS_MAX     (100)
#define STEP7_GAP_BALANCE_BIAS_STEP    (5)
#define STEP7_GAP_BLACK_CONFIRM_TICKS  (3U)

#define ER2024_FW_BUILD_TAG            "2026-07-02-auto-direct-pid"

#define AUTO_GAP_BLACK_MIN_TICKS       (1200)
#define AUTO_GAP_MISS_TICKS            (1900)
#define AUTO_GAP_BLACK_CONFIRM_TICKS   (1U)
#define AUTO_BC_EXIT_MIN_TICKS         (1500)
#define AUTO_BC_FORCE_EXIT_TICKS       (2500)
#define AUTO_DA_EXIT_MIN_TICKS         (1800)
#define AUTO_DA_FORCE_EXIT_TICKS       (2800)
#define AUTO_ARC_ENTRY_GRACE_TICKS     (800)
#define AUTO_ARC_LOST_CONFIRM_TICKS    (3U)

/*
 * Confirmed 2026-07-01:
 *   - HW-A/AO1-AO2 controls the physical right wheel;
 *   - HW-B/BO1-BO2 controls the physical left wheel;
 *   - logical forward is the opposite of the previous Step 5 check.
 *
 * These defaults correct the logical car frame:
 *   - logical left/right are swapped across the two TB6612 channels;
 *   - both motor channels are inverted so logical forward is current forward;
 *   - sensor and encoder left/right signs are corrected together.
 *
 * If the bench test shows one item is still wrong, use UART commands below or
 * change the matching default from 1 to 0.
 */
#define STEP6_SWAP_MOTOR_SIDES_DEFAULT    (1U)
#define STEP6_INVERT_MOTOR_A_DEFAULT      (1U)
#define STEP6_INVERT_MOTOR_B_DEFAULT      (1U)
#define STEP6_REVERSE_SENSOR_LR_DEFAULT   (1U)
#define STEP6_SWAP_ENCODER_SIDES_DEFAULT  (1U)
#define STEP6_INVERT_ENCODER_A_DEFAULT    (1U)
#define STEP6_INVERT_ENCODER_B_DEFAULT    (1U)

typedef enum {
    DRIVE_STANDBY = 0,
    DRIVE_RUN,
    DRIVE_BRAKE,
    DRIVE_JOG_FWD,
    DRIVE_JOG_REV,
    DRIVE_GAP_TO_BLACK,
    DRIVE_AUTO
} drive_mode_t;

typedef enum {
    LINE_STATUS_OK = 0,
    LINE_STATUS_LOST,
    LINE_STATUS_ALL_BLACK
} line_status_t;

typedef enum {
    GAP_TEST_OFF = 0,
    GAP_TEST_WAIT_WHITE,
    GAP_TEST_RUNNING,
    GAP_TEST_DONE
} gap_test_state_t;

typedef enum {
    AUTO_SEG_OFF = 0,
    AUTO_SEG_AB,
    AUTO_SEG_BC,
    AUTO_SEG_CD,
    AUTO_SEG_DA,
    AUTO_SEG_DONE,
    AUTO_SEG_FAIL
} auto_segment_t;

typedef struct {
    uint8_t       raw;
    uint8_t       activeCount;
    int16_t       error;
    line_status_t status;
} line_position_t;

#define LINE_SENSOR_ADDR_PIN_MASK                                          \
    (GPIO_LINE_ADDR_AD0_PIN | GPIO_LINE_ADDR_AD1_PIN | GPIO_LINE_ADDR_AD2_PIN)

/* ---- hardware-side encoder globals (modified in ISR) ---- */
static volatile int32_t g_encA = 0; /* ENC1/PB17-PB18, same side as PWMA/AO */
static volatile int32_t g_encB = 0; /* ENC2/PB24-PB25, same side as PWMB/BO */

static volatile uint8_t g_uartCommand = 0U;

static drive_mode_t  g_driveMode      = DRIVE_STANDBY;
static line_status_t g_lastLineStatus = LINE_STATUS_LOST;
static uint8_t       g_lastLineRaw    = 0U;
static uint8_t       g_lastLineActive = 0U;
static int16_t       g_lastLineError  = 0;
static int16_t       g_pidPrevError   = 0;
static int32_t       g_pidIntegral    = 0;
static int16_t       g_baseSpeed      = STEP6_BASE_SPEED_DEFAULT;
static int16_t       g_pidKp          = STEP6_KP_DEFAULT;
static int16_t       g_pidKi          = STEP6_KI_DEFAULT;
static int16_t       g_pidKd          = STEP6_KD_DEFAULT;
static int16_t       g_lastCorrection = 0;
static int16_t       g_lastLeftCmd    = 0;
static int16_t       g_lastRightCmd   = 0;
static int32_t       g_lapTargetTicks = STEP7_LAP_TARGET_TICKS_DEFAULT;
static int32_t       g_lastOdomTicks  = 0;
static int32_t       g_gapStopOdomTicks = 0;
static uint16_t      g_lineLostTicks  = 0U;
static uint8_t       g_lapActive      = 0U;
static uint8_t       g_lapDone        = 0U;
static uint8_t       g_gapStopRaw     = 0U;
static uint8_t       g_gapBlackTicks  = 0U;
static int16_t       g_gapBalanceBias = STEP7_GAP_BALANCE_BIAS_DEFAULT;
static int32_t       g_gapPrevLeftTicks  = 0;
static int32_t       g_gapPrevRightTicks = 0;
static int32_t       g_gapLastDeltaLeft  = 0;
static int32_t       g_gapLastDeltaRight = 0;
static gap_test_state_t g_gapTestState = GAP_TEST_OFF;
static auto_segment_t   g_autoSegment  = AUTO_SEG_OFF;
static int32_t       g_autoSegStartOdomTicks = 0;
static int32_t       g_autoSegOdomTicks = 0;
static uint8_t       g_autoLostTicks    = 0U;
static uint8_t       g_autoStraightReady = 0U;

static uint8_t g_motorSwapSides   = STEP6_SWAP_MOTOR_SIDES_DEFAULT;
static uint8_t g_motorInvertA     = STEP6_INVERT_MOTOR_A_DEFAULT;
static uint8_t g_motorInvertB     = STEP6_INVERT_MOTOR_B_DEFAULT;
static uint8_t g_sensorReverseLR  = STEP6_REVERSE_SENSOR_LR_DEFAULT;
static uint8_t g_encoderSwapSides = STEP6_SWAP_ENCODER_SIDES_DEFAULT;
static uint8_t g_encoderInvertA   = STEP6_INVERT_ENCODER_A_DEFAULT;
static uint8_t g_encoderInvertB   = STEP6_INVERT_ENCODER_B_DEFAULT;

static void linePid_reset(void);
static int16_t clampInt16(int32_t value, int16_t minVal, int16_t maxVal);
static void step7_clearAutoRun(void);
static void step7_startAutoRun(void);

static uint16_t motorPwmCompareFromSpeed(int16_t speed)
{
    int32_t duty = speed;

    if (duty < 0) {
        duty = -duty;
    }
    if (duty > PWM_MAX) {
        duty = PWM_MAX;
    }

    return (uint16_t)(PWM_MAX - duty);
}

/* ---- delay helpers ---- */
static void delay_ms(uint32_t ms)
{
    delay_cycles((CPUCLK_FREQ / 1000U) * ms);
}

static void delay_us(uint32_t us)
{
    delay_cycles((CPUCLK_FREQ / 1000000U) * us);
}

/* ---- UART helpers ---- */
static void UART0_writeByte(uint8_t data)
{
    while (DL_UART_Main_isTXFIFOFull(UART_0_INST)) {
    }
    DL_UART_Main_transmitData(UART_0_INST, data);
}

static void UART0_writeString(const char *str)
{
    while (*str != '\0') {
        UART0_writeByte((uint8_t)*str);
        str++;
    }
}

static void UART0_writeBinary8(uint8_t value)
{
    for (int8_t bit = 7; bit >= 0; bit--) {
        UART0_writeByte((uint8_t)((value & (1U << bit)) ? '1' : '0'));
    }
}

static void UART0_writeDec32(int32_t val)
{
    char buf[12];
    int  i   = 0;
    uint32_t u;

    if (val < 0) {
        UART0_writeByte('-');
        u = (uint32_t)(-val);
    } else {
        u = (uint32_t)val;
    }

    do {
        buf[i++] = (char)('0' + (u % 10U));
        u /= 10U;
    } while (u != 0U);

    while (i > 0) {
        UART0_writeByte((uint8_t)buf[--i]);
    }
}

static const char *driveModeName(drive_mode_t mode)
{
    switch (mode) {
        case DRIVE_RUN:
            return "RUN";
        case DRIVE_BRAKE:
            return "BRAKE";
        case DRIVE_JOG_FWD:
            return "FWD";
        case DRIVE_JOG_REV:
            return "REV";
        case DRIVE_GAP_TO_BLACK:
            return "GAP";
        case DRIVE_AUTO:
            return "AUTO";
        case DRIVE_STANDBY:
        default:
            return "STBY";
    }
}

static const char *lineStatusName(line_status_t status)
{
    switch (status) {
        case LINE_STATUS_OK:
            return "OK";
        case LINE_STATUS_ALL_BLACK:
            return "FULL";
        case LINE_STATUS_LOST:
        default:
            return "LOST";
    }
}

static const char *gapTestStateName(gap_test_state_t state)
{
    switch (state) {
        case GAP_TEST_WAIT_WHITE:
            return "WAIT";
        case GAP_TEST_RUNNING:
            return "RUN";
        case GAP_TEST_DONE:
            return "DONE";
        case GAP_TEST_OFF:
        default:
            return "OFF";
    }
}

static const char *autoSegmentName(auto_segment_t segment)
{
    switch (segment) {
        case AUTO_SEG_AB:
            return "AB";
        case AUTO_SEG_BC:
            return "BC";
        case AUTO_SEG_CD:
            return "CD";
        case AUTO_SEG_DA:
            return "DA";
        case AUTO_SEG_DONE:
            return "DONE";
        case AUTO_SEG_FAIL:
            return "FAIL";
        case AUTO_SEG_OFF:
            return "OFF";
        default:
            return "BAD";
    }
}

static uint8_t autoSegmentIsRunSegment(auto_segment_t segment)
{
    switch (segment) {
        case AUTO_SEG_AB:
        case AUTO_SEG_BC:
        case AUTO_SEG_CD:
        case AUTO_SEG_DA:
            return 1U;
        default:
            return 0U;
    }
}

static void step7_setAutoSegment(auto_segment_t segment)
{
    g_autoSegment = segment;
}

/* ---- line sensor ---- */
static void lineSensor_selectChannel(uint8_t channel)
{
    uint32_t pins = 0U;

    if ((channel & 0x01U) != 0U) {
        pins |= GPIO_LINE_ADDR_AD0_PIN;
    }
    if ((channel & 0x02U) != 0U) {
        pins |= GPIO_LINE_ADDR_AD1_PIN;
    }
    if ((channel & 0x04U) != 0U) {
        pins |= GPIO_LINE_ADDR_AD2_PIN;
    }

    DL_GPIO_clearPins(GPIO_LINE_ADDR_PORT, LINE_SENSOR_ADDR_PIN_MASK);
    DL_GPIO_setPins(GPIO_LINE_ADDR_PORT, pins);
    delay_us(LINE_SENSOR_SETTLE_US);
}

static uint8_t lineSensor_readOutBit(void)
{
    return (DL_GPIO_readPins(GPIO_LINE_OUT_PORT, GPIO_LINE_OUT_OUT_PIN) != 0U)
               ? 1U
               : 0U;
}

static uint8_t lineSensor_readMuxedRaw(void)
{
    uint8_t raw = 0U;

    for (uint8_t channel = 0U; channel < LINE_SENSOR_CHANNEL_COUNT; channel++) {
        lineSensor_selectChannel(channel);
        raw |= (uint8_t)(lineSensor_readOutBit() << channel);
    }

    return raw;
}

static line_position_t lineSensor_readPosition(void)
{
    /*
     * Base board order from the old check: bit0=X1 was the old physical right
     * side, bit7=X8 was the old left side. g_sensorReverseLR flips this after
     * correcting the chassis front direction. Positive error always means the
     * black line is to the logical right, so logical left wheel runs faster.
     */
    static const int16_t weights[LINE_SENSOR_CHANNEL_COUNT] = {
        350, 250, 150, 50, -50, -150, -250, -350
    };

    line_position_t pos;
    int32_t         weightedSum = 0;

    pos.raw         = lineSensor_readMuxedRaw();
    pos.activeCount = 0U;
    pos.error       = 0;
    pos.status      = LINE_STATUS_OK;

    for (uint8_t channel = 0U; channel < LINE_SENSOR_CHANNEL_COUNT; channel++) {
        if ((pos.raw & (1U << channel)) != 0U) {
            weightedSum += (g_sensorReverseLR != 0U) ? -weights[channel]
                                                     : weights[channel];
            pos.activeCount++;
        }
    }

    if (pos.activeCount == 0U) {
        pos.status = LINE_STATUS_LOST;
    } else if (pos.activeCount == LINE_SENSOR_CHANNEL_COUNT) {
        pos.status = LINE_STATUS_ALL_BLACK;
    } else {
        pos.error = (int16_t)(weightedSum / (int32_t)pos.activeCount);
    }

    return pos;
}

/* ---- motor control ---- */
static void motorHwA(int16_t speed)
{
    if (speed > 0) {
        DL_GPIO_clearPins(GPIO_MOTOR_DIR_PORT, GPIO_MOTOR_DIR_AIN1_PIN);
        DL_GPIO_setPins(GPIO_MOTOR_DIR_PORT, GPIO_MOTOR_DIR_AIN2_PIN);
        DL_TimerA_setCaptureCompareValue(PWM_MOTORS_INST,
                                         motorPwmCompareFromSpeed(speed),
                                         GPIO_PWM_MOTORS_C0_IDX);
    } else if (speed < 0) {
        DL_GPIO_setPins(GPIO_MOTOR_DIR_PORT, GPIO_MOTOR_DIR_AIN1_PIN);
        DL_GPIO_clearPins(GPIO_MOTOR_DIR_PORT, GPIO_MOTOR_DIR_AIN2_PIN);
        DL_TimerA_setCaptureCompareValue(PWM_MOTORS_INST,
                                         motorPwmCompareFromSpeed(speed),
                                         GPIO_PWM_MOTORS_C0_IDX);
    } else {
        DL_GPIO_clearPins(GPIO_MOTOR_DIR_PORT,
                          GPIO_MOTOR_DIR_AIN1_PIN | GPIO_MOTOR_DIR_AIN2_PIN);
        DL_TimerA_setCaptureCompareValue(PWM_MOTORS_INST,
                                         motorPwmCompareFromSpeed(speed),
                                         GPIO_PWM_MOTORS_C0_IDX);
    }
}

static void motorHwB(int16_t speed)
{
    if (speed > 0) {
        DL_GPIO_setPins(GPIO_MOTOR_DIR_PORT, GPIO_MOTOR_DIR_BIN1_PIN);
        DL_GPIO_clearPins(GPIO_MOTOR_DIR_PORT, GPIO_MOTOR_DIR_BIN2_PIN);
        DL_TimerA_setCaptureCompareValue(PWM_MOTORS_INST,
                                         motorPwmCompareFromSpeed(speed),
                                         GPIO_PWM_MOTORS_C1_IDX);
    } else if (speed < 0) {
        DL_GPIO_clearPins(GPIO_MOTOR_DIR_PORT, GPIO_MOTOR_DIR_BIN1_PIN);
        DL_GPIO_setPins(GPIO_MOTOR_DIR_PORT, GPIO_MOTOR_DIR_BIN2_PIN);
        DL_TimerA_setCaptureCompareValue(PWM_MOTORS_INST,
                                         motorPwmCompareFromSpeed(speed),
                                         GPIO_PWM_MOTORS_C1_IDX);
    } else {
        DL_GPIO_clearPins(GPIO_MOTOR_DIR_PORT,
                          GPIO_MOTOR_DIR_BIN1_PIN | GPIO_MOTOR_DIR_BIN2_PIN);
        DL_TimerA_setCaptureCompareValue(PWM_MOTORS_INST,
                                         motorPwmCompareFromSpeed(speed),
                                         GPIO_PWM_MOTORS_C1_IDX);
    }
}

static void motorStop(void)
{
    motorHwA(0);
    motorHwB(0);
}

static void motorBrake(void)
{
    DL_GPIO_setPins(GPIO_MOTOR_DIR_PORT,
                    GPIO_MOTOR_DIR_AIN1_PIN | GPIO_MOTOR_DIR_AIN2_PIN |
                        GPIO_MOTOR_DIR_BIN1_PIN | GPIO_MOTOR_DIR_BIN2_PIN);
    DL_TimerA_setCaptureCompareValue(PWM_MOTORS_INST,
                                     motorPwmCompareFromSpeed(PWM_MAX),
                                     GPIO_PWM_MOTORS_C0_IDX);
    DL_TimerA_setCaptureCompareValue(PWM_MOTORS_INST,
                                     motorPwmCompareFromSpeed(PWM_MAX),
                                     GPIO_PWM_MOTORS_C1_IDX);
}

static int16_t maybeInvertMotorCommand(int16_t value, uint8_t invert)
{
    return (invert != 0U) ? (int16_t)-value : value;
}

static void driveSetLogical(int16_t left, int16_t right)
{
    int16_t hwA = left;
    int16_t hwB = right;

    if (g_motorSwapSides != 0U) {
        hwA = right;
        hwB = left;
    }

    hwA = maybeInvertMotorCommand(hwA, g_motorInvertA);
    hwB = maybeInvertMotorCommand(hwB, g_motorInvertB);

    motorHwA(hwA);
    motorHwB(hwB);
}

static int32_t maybeInvertEncoderCount(int32_t value, uint8_t invert)
{
    return (invert != 0U) ? -value : value;
}

static void step6_readLogicalEncoders(int32_t *left, int32_t *right)
{
    int32_t encA = maybeInvertEncoderCount(g_encA, g_encoderInvertA);
    int32_t encB = maybeInvertEncoderCount(g_encB, g_encoderInvertB);

    if (g_encoderSwapSides != 0U) {
        *left  = encB;
        *right = encA;
    } else {
        *left  = encA;
        *right = encB;
    }
}

static int32_t step7_getOdomTicks(void)
{
    int32_t left;
    int32_t right;

    step6_readLogicalEncoders(&left, &right);

    if (left < 0) {
        left = 0;
    }
    if (right < 0) {
        right = 0;
    }

    return (left + right) / 2;
}

static int16_t step7_gapStraightCorrection(void)
{
    int32_t left;
    int32_t right;
    int32_t positionError;
    int32_t speedError;
    int32_t correction;
    int32_t deltaLeft;
    int32_t deltaRight;

    step6_readLogicalEncoders(&left, &right);

    if (left < 0) {
        left = 0;
    }
    if (right < 0) {
        right = 0;
    }

    deltaLeft = left - g_gapPrevLeftTicks;
    deltaRight = right - g_gapPrevRightTicks;
    if (deltaLeft < 0) {
        deltaLeft = 0;
    }
    if (deltaRight < 0) {
        deltaRight = 0;
    }

    g_gapPrevLeftTicks = left;
    g_gapPrevRightTicks = right;
    g_gapLastDeltaLeft = deltaLeft;
    g_gapLastDeltaRight = deltaRight;

    positionError = left - right;
    speedError = deltaLeft - deltaRight;

    correction = (positionError * STEP7_GAP_BALANCE_POS_KP_NUM) /
                 STEP7_GAP_BALANCE_POS_KP_DEN;
    correction += (speedError * STEP7_GAP_BALANCE_SPEED_KP_NUM) /
                  STEP7_GAP_BALANCE_SPEED_KP_DEN;

    correction += g_gapBalanceBias;

    return clampInt16(correction,
                      -STEP7_GAP_BALANCE_MAX,
                      STEP7_GAP_BALANCE_MAX);
}

static void step7_gapResetStraightController(void)
{
    int32_t left;
    int32_t right;

    step6_readLogicalEncoders(&left, &right);

    if (left < 0) {
        left = 0;
    }
    if (right < 0) {
        right = 0;
    }

    g_gapPrevLeftTicks = left;
    g_gapPrevRightTicks = right;
    g_gapLastDeltaLeft = 0;
    g_gapLastDeltaRight = 0;
}

static void step7_resetOdom(void)
{
    g_encA          = 0;
    g_encB          = 0;
    g_lastOdomTicks = 0;
    g_lineLostTicks = 0U;
}

static void step7_clearGapTest(void)
{
    g_gapTestState     = GAP_TEST_OFF;
    g_gapStopOdomTicks = 0;
    g_gapStopRaw       = 0U;
    g_gapBlackTicks    = 0U;
    g_gapLastDeltaLeft = 0;
    g_gapLastDeltaRight = 0;
}

static void step7_printLapParams(void)
{
    UART0_writeString("lapTarget=");
    UART0_writeDec32(g_lapTargetTicks);
    UART0_writeString(" odom=");
    UART0_writeDec32(g_lastOdomTicks);
    UART0_writeString(" gapBias=");
    UART0_writeDec32(g_gapBalanceBias);
    UART0_writeString(" auto=");
    if (g_autoSegment == AUTO_SEG_DONE) {
        UART0_writeString("DONE");
    } else if (g_autoSegment == AUTO_SEG_FAIL) {
        UART0_writeString("FAIL");
    } else if (g_autoSegment == AUTO_SEG_OFF) {
        UART0_writeString("OFF");
    } else if (autoSegmentIsRunSegment(g_autoSegment) != 0U) {
        UART0_writeString("RUN");
    } else {
        UART0_writeString("BAD");
    }
    UART0_writeString(" seg=");
    UART0_writeString(autoSegmentName(g_autoSegment));
    UART0_writeString(" segOdom=");
    UART0_writeDec32(g_autoSegOdomTicks);
    UART0_writeString(" autoReady=");
    UART0_writeDec32(g_autoStraightReady);
    UART0_writeString("\r\n");
}

static void step7_startLapRun(void)
{
    step7_clearAutoRun();
    step7_clearGapTest();
    step7_resetOdom();
    g_lapActive = 1U;
    g_lapDone   = 0U;
    g_driveMode = DRIVE_RUN;
    linePid_reset();

    UART0_writeString("\r\n[RUN] Step7 one-lap run; target=");
    UART0_writeDec32(g_lapTargetTicks);
    UART0_writeString(" ticks\r\n");
}

static void step7_startGapToBlackRun(void)
{
    step7_clearAutoRun();
    step7_resetOdom();
    g_lapActive = 0U;
    g_lapDone   = 0U;
    g_driveMode = DRIVE_GAP_TO_BLACK;
    g_gapTestState = GAP_TEST_WAIT_WHITE;
    g_gapStopOdomTicks = 0;
    g_gapStopRaw = 0U;
    g_gapBlackTicks = 0U;
    step7_gapResetStraightController();
    linePid_reset();

    UART0_writeString("\r\n[GAP] armed; wait white, then run until confirmed black\r\n");
}

static uint8_t step7_checkLapStop(void)
{
    if ((g_driveMode != DRIVE_RUN) || (g_lapActive == 0U)) {
        g_lastOdomTicks = step7_getOdomTicks();
        return 0U;
    }

    g_lastOdomTicks = step7_getOdomTicks();
    if (g_lastOdomTicks < g_lapTargetTicks) {
        return 0U;
    }

    g_driveMode = DRIVE_BRAKE;
    g_lapActive = 0U;
    g_lapDone   = 1U;
    motorBrake();
    linePid_reset();

    UART0_writeString("\r\n[LAP] target reached; brake at odom=");
    UART0_writeDec32(g_lastOdomTicks);
    UART0_writeString("\r\n");

    return 1U;
}

static int16_t clampInt16(int32_t value, int16_t minVal, int16_t maxVal)
{
    if (value < (int32_t)minVal) {
        return minVal;
    }
    if (value > (int32_t)maxVal) {
        return maxVal;
    }
    return (int16_t)value;
}

static void linePid_reset(void)
{
    g_pidPrevError   = 0;
    g_pidIntegral    = 0;
    g_lastCorrection = 0;
    g_lastLeftCmd    = 0;
    g_lastRightCmd   = 0;
    g_lineLostTicks  = 0U;
}

static int16_t linePid_calculate(int16_t error)
{
    int32_t derivative;
    int32_t output;

    g_pidIntegral += error;
    g_pidIntegral = clampInt16(g_pidIntegral,
                               -STEP6_INTEGRAL_LIMIT,
                               STEP6_INTEGRAL_LIMIT);

    derivative     = (int32_t)error - (int32_t)g_pidPrevError;
    g_pidPrevError = error;

    output = ((int32_t)g_pidKp * error) + ((int32_t)g_pidKi * g_pidIntegral) +
             ((int32_t)g_pidKd * derivative);
    output /= STEP6_PID_SCALE;

    return clampInt16(output, -STEP6_STEER_MAX, STEP6_STEER_MAX);
}

static void step7_gapDriveStraight(void)
{
    g_lastCorrection = step7_gapStraightCorrection();
    g_lastLeftCmd =
        clampInt16((int32_t)g_baseSpeed - g_lastCorrection, 0, PWM_MAX);
    g_lastRightCmd =
        clampInt16((int32_t)g_baseSpeed + g_lastCorrection, 0, PWM_MAX);
    driveSetLogical(g_lastLeftCmd, g_lastRightCmd);
}

static uint8_t step7_gapWhiteToBlackUpdate(const line_position_t *line,
                                           int32_t minBlackOdomTicks,
                                           uint8_t blackConfirmTicks)
{
    g_lastOdomTicks = step7_getOdomTicks();
    g_lastCorrection = 0;

    if (g_gapTestState == GAP_TEST_WAIT_WHITE) {
        g_lastLeftCmd  = 0;
        g_lastRightCmd = 0;
        motorStop();

        if (line->activeCount == 0U) {
            step7_resetOdom();
            step7_gapResetStraightController();
            g_lastOdomTicks = 0;
            g_gapTestState = GAP_TEST_RUNNING;
            g_gapBlackTicks = 0U;
            UART0_writeString("\r\n[GAP] white detected; odom reset and run\r\n");
        }
        return 0U;
    }

    if (g_gapTestState != GAP_TEST_RUNNING) {
        return 0U;
    }

    if (line->activeCount == 0U) {
        g_gapBlackTicks = 0U;
        step7_gapDriveStraight();
        return 0U;
    }

    if (g_lastOdomTicks < minBlackOdomTicks) {
        g_gapBlackTicks = 0U;
        step7_gapDriveStraight();
        return 0U;
    }

    if (g_gapBlackTicks < blackConfirmTicks) {
        g_gapBlackTicks++;
    }

    if (g_gapBlackTicks < blackConfirmTicks) {
        step7_gapDriveStraight();
        return 0U;
    }

    g_gapStopOdomTicks = g_lastOdomTicks;
    g_gapStopRaw = line->raw;
    return 1U;
}

static void step7_driveWithLastLineCorrection(void)
{
    g_lastLeftCmd =
        clampInt16((int32_t)g_baseSpeed + g_lastCorrection, 0, PWM_MAX);
    g_lastRightCmd =
        clampInt16((int32_t)g_baseSpeed - g_lastCorrection, 0, PWM_MAX);
    driveSetLogical(g_lastLeftCmd, g_lastRightCmd);
}

static void step7_driveLinePid(const line_position_t *line)
{
    g_lastCorrection = linePid_calculate(line->error);
    step7_driveWithLastLineCorrection();
}

static void step7_clearAutoRun(void)
{
    step7_setAutoSegment(AUTO_SEG_OFF);
    g_autoSegStartOdomTicks = 0;
    g_autoSegOdomTicks      = 0;
    g_autoLostTicks         = 0U;
    g_autoStraightReady     = 0U;
}

static void step7_autoPrintSegment(const char *prefix)
{
    g_lastOdomTicks = step7_getOdomTicks();
    g_autoSegOdomTicks = g_lastOdomTicks - g_autoSegStartOdomTicks;
    if (g_autoSegOdomTicks < 0) {
        g_autoSegOdomTicks = 0;
    }

    UART0_writeString("\r\n[AUTO] ");
    UART0_writeString(prefix);
    UART0_writeString(" seg=");
    UART0_writeString(autoSegmentName(g_autoSegment));
    UART0_writeString(" odom=");
    UART0_writeDec32(g_lastOdomTicks);
    UART0_writeString(" segOdom=");
    UART0_writeDec32(g_autoSegOdomTicks);
    UART0_writeString(" autoReady=");
    UART0_writeDec32(g_autoStraightReady);
    UART0_writeString("\r\n");
}

static void step7_autoEnterSegment(auto_segment_t segment)
{
    step7_setAutoSegment(segment);
    g_driveMode             = DRIVE_AUTO;
    g_autoSegStartOdomTicks = step7_getOdomTicks();
    g_autoSegOdomTicks      = 0;
    g_autoLostTicks         = 0U;
    g_gapBlackTicks         = 0U;
    g_lineLostTicks         = 0U;
    g_autoStraightReady     =
        ((segment == AUTO_SEG_AB) || (segment == AUTO_SEG_CD)) ? 0U : 1U;
    if ((segment == AUTO_SEG_AB) || (segment == AUTO_SEG_CD)) {
        g_gapTestState     = GAP_TEST_WAIT_WHITE;
        g_gapStopOdomTicks = 0;
        g_gapStopRaw       = 0U;
    } else {
        g_gapTestState = GAP_TEST_OFF;
    }
    linePid_reset();

    step7_autoPrintSegment("enter");
}

static void step7_autoPrintTransition(auto_segment_t nextSegment)
{
    UART0_writeString("\r\n[AUTO] transition to=");
    UART0_writeString(autoSegmentName(nextSegment));
    UART0_writeString(" code=");
    UART0_writeDec32((int32_t)nextSegment);
    UART0_writeString(" from=");
    UART0_writeString(autoSegmentName(g_autoSegment));
    UART0_writeString(" segOdom=");
    UART0_writeDec32(g_autoSegOdomTicks);
    UART0_writeString("\r\n");
}

static void step7_autoBrake(auto_segment_t terminalSegment, const char *reason)
{
    g_driveMode     = DRIVE_BRAKE;
    step7_setAutoSegment(terminalSegment);
    g_lapActive     = 0U;
    g_lapDone       = (terminalSegment == AUTO_SEG_DONE) ? 1U : 0U;
    g_lastLeftCmd   = 0;
    g_lastRightCmd  = 0;
    motorBrake();
    linePid_reset();

    UART0_writeString("\r\n[AUTO] ");
    UART0_writeString(reason);
    UART0_writeString(" at segOdom=");
    UART0_writeDec32(g_autoSegOdomTicks);
    UART0_writeString(" odom=");
    UART0_writeDec32(g_lastOdomTicks);
    UART0_writeString("\r\n");
}

static void step7_startAutoRun(void)
{
    step7_clearGapTest();
    step7_resetOdom();
    step7_gapResetStraightController();
    g_lapActive = 0U;
    g_lapDone   = 0U;
    g_driveMode = DRIVE_AUTO;
    step7_setAutoSegment(AUTO_SEG_AB);
    g_autoSegStartOdomTicks = 0;
    g_autoSegOdomTicks = 0;
    g_autoLostTicks = 0U;
    g_autoStraightReady = 0U;
    g_gapBlackTicks = 0U;
    g_gapTestState = GAP_TEST_WAIT_WHITE;
    g_gapStopOdomTicks = 0;
    g_gapStopRaw = 0U;
    linePid_reset();

    UART0_writeString("\r\n[AUTO] A-B-C-D-A segmented run armed\r\n");
    step7_autoPrintSegment("enter");
}

static void step7_autoTransition(auto_segment_t nextSegment)
{
    int32_t nextCode = (int32_t)nextSegment;

    step7_autoPrintTransition(nextSegment);

    if (nextSegment == AUTO_SEG_DONE) {
        step7_autoBrake(AUTO_SEG_DONE, "done");
    } else if ((nextCode <= (int32_t)AUTO_SEG_OFF) ||
               (nextSegment == AUTO_SEG_FAIL) ||
               (nextCode > (int32_t)AUTO_SEG_FAIL)) {
        step7_autoBrake(AUTO_SEG_FAIL, "bad transition");
    } else {
        step7_autoEnterSegment(nextSegment);
    }
}

static uint8_t step7_autoStraightUpdate(const line_position_t *line)
{
    uint8_t blackConfirmed =
        step7_gapWhiteToBlackUpdate(line,
                                    AUTO_GAP_BLACK_MIN_TICKS,
                                    AUTO_GAP_BLACK_CONFIRM_TICKS);

    g_autoStraightReady =
        (g_gapTestState == GAP_TEST_RUNNING) ? 1U : 0U;
    g_autoSegStartOdomTicks = 0;
    g_autoSegOdomTicks = g_lastOdomTicks;

    if (blackConfirmed != 0U) {
        g_gapTestState = GAP_TEST_DONE;
        return 1U;
    }

    if ((g_autoStraightReady != 0U) &&
        (g_autoSegOdomTicks >= AUTO_GAP_MISS_TICKS)) {
        step7_autoBrake(AUTO_SEG_FAIL, "gap missed black");
        return 0U;
    }

    return 0U;
}

static uint8_t step7_autoArcUpdate(const line_position_t *line,
                                   int32_t exitMinTicks,
                                   int32_t forceExitTicks)
{
    if (line->status == LINE_STATUS_ALL_BLACK) {
        g_autoLostTicks = 0U;
        g_lineLostTicks = 0U;
        step7_driveLinePid(line);
        return 0U;
    }

    if (line->status == LINE_STATUS_LOST) {
        if (g_autoSegOdomTicks < AUTO_ARC_ENTRY_GRACE_TICKS) {
            g_autoLostTicks = 0U;
            step7_driveWithLastLineCorrection();
            return 0U;
        }

        if (g_autoSegOdomTicks >= exitMinTicks) {
            if (g_autoLostTicks < AUTO_ARC_LOST_CONFIRM_TICKS) {
                g_autoLostTicks++;
            }
            if (g_autoLostTicks >= AUTO_ARC_LOST_CONFIRM_TICKS) {
                return 1U;
            }
        } else if (g_lineLostTicks < STEP7_LOST_GRACE_TICKS) {
            g_lineLostTicks++;
        } else {
            step7_autoBrake(AUTO_SEG_FAIL, "arc lost early");
            return 0U;
        }

        step7_driveWithLastLineCorrection();
        return 0U;
    }

    g_autoLostTicks = 0U;

    if (g_autoSegOdomTicks >= forceExitTicks) {
        return 1U;
    }

    g_lineLostTicks = 0U;
    step7_driveLinePid(line);
    return 0U;
}

static void step7_autoUpdate(const line_position_t *line)
{
    g_lastOdomTicks = step7_getOdomTicks();
    g_autoSegOdomTicks = g_lastOdomTicks - g_autoSegStartOdomTicks;
    if (g_autoSegOdomTicks < 0) {
        g_autoSegOdomTicks = 0;
    }

    switch (g_autoSegment) {
        case AUTO_SEG_AB:
            if (step7_autoStraightUpdate(line) != 0U) {
                step7_driveLinePid(line);
                step7_autoTransition(AUTO_SEG_BC);
                step7_driveLinePid(line);
            }
            break;
        case AUTO_SEG_BC:
            if (step7_autoArcUpdate(line,
                                    AUTO_BC_EXIT_MIN_TICKS,
                                    AUTO_BC_FORCE_EXIT_TICKS) != 0U) {
                step7_autoTransition(AUTO_SEG_CD);
            }
            break;
        case AUTO_SEG_CD:
            if (step7_autoStraightUpdate(line) != 0U) {
                step7_driveLinePid(line);
                step7_autoTransition(AUTO_SEG_DA);
                step7_driveLinePid(line);
            }
            break;
        case AUTO_SEG_DA:
            if (step7_autoArcUpdate(line,
                                    AUTO_DA_EXIT_MIN_TICKS,
                                    AUTO_DA_FORCE_EXIT_TICKS) != 0U) {
                step7_autoTransition(AUTO_SEG_DONE);
            }
            break;
        case AUTO_SEG_DONE:
            step7_autoBrake(AUTO_SEG_DONE, "done");
            break;
        case AUTO_SEG_FAIL:
        case AUTO_SEG_OFF:
        default:
            step7_autoBrake(AUTO_SEG_FAIL, "invalid state");
            break;
    }
}

static void step6_lineFollowUpdate(void)
{
    line_position_t line = lineSensor_readPosition();

    g_lastLineRaw    = line.raw;
    g_lastLineActive = line.activeCount;
    g_lastLineError  = line.error;
    g_lastLineStatus = line.status;

    if (g_driveMode == DRIVE_STANDBY) {
        motorStop();
        linePid_reset();
        g_lastOdomTicks = step7_getOdomTicks();
        return;
    }

    if (g_driveMode == DRIVE_BRAKE) {
        motorBrake();
        linePid_reset();
        g_lastOdomTicks = step7_getOdomTicks();
        return;
    }

    if (g_driveMode == DRIVE_JOG_FWD) {
        g_lastCorrection = 0;
        g_lastLeftCmd    = g_baseSpeed;
        g_lastRightCmd   = g_baseSpeed;
        driveSetLogical(g_lastLeftCmd, g_lastRightCmd);
        g_lastOdomTicks = step7_getOdomTicks();
        return;
    }

    if (g_driveMode == DRIVE_JOG_REV) {
        g_lastCorrection = 0;
        g_lastLeftCmd    = -g_baseSpeed;
        g_lastRightCmd   = -g_baseSpeed;
        driveSetLogical(g_lastLeftCmd, g_lastRightCmd);
        g_lastOdomTicks = step7_getOdomTicks();
        return;
    }

    if (g_driveMode == DRIVE_AUTO) {
        step7_autoUpdate(&line);
        return;
    }

    if (g_driveMode == DRIVE_GAP_TO_BLACK) {
        if (g_gapTestState == GAP_TEST_DONE) {
            motorBrake();
            g_lastLeftCmd  = 0;
            g_lastRightCmd = 0;
            return;
        }

        if (step7_gapWhiteToBlackUpdate(&line,
                                        0,
                                        STEP7_GAP_BLACK_CONFIRM_TICKS) == 0U) {
            return;
        }

        g_driveMode    = DRIVE_BRAKE;
        g_gapTestState = GAP_TEST_DONE;
        g_lastLeftCmd  = 0;
        g_lastRightCmd = 0;
        motorBrake();
        linePid_reset();

        UART0_writeString("\r\n[GAP] black detected; brake at odom=");
        UART0_writeDec32(g_lastOdomTicks);
        UART0_writeString(" raw=0b");
        UART0_writeBinary8(line.raw);
        UART0_writeString("\r\n");
        return;
    }

    if (step7_checkLapStop() != 0U) {
        return;
    }

    if (line.status == LINE_STATUS_ALL_BLACK) {
        motorBrake();
        g_lastCorrection = 0;
        g_lastLeftCmd    = 0;
        g_lastRightCmd   = 0;
        linePid_reset();
        return;
    }

    if (line.status == LINE_STATUS_LOST) {
        if (g_lineLostTicks < STEP7_LOST_GRACE_TICKS) {
            g_lineLostTicks++;
            g_lastLeftCmd =
                clampInt16((int32_t)g_baseSpeed + g_lastCorrection, 0, PWM_MAX);
            g_lastRightCmd =
                clampInt16((int32_t)g_baseSpeed - g_lastCorrection, 0, PWM_MAX);
            driveSetLogical(g_lastLeftCmd, g_lastRightCmd);
            return;
        }

        motorBrake();
        g_lastCorrection = 0;
        g_lastLeftCmd    = 0;
        g_lastRightCmd   = 0;
        linePid_reset();
        g_lineLostTicks = STEP7_LOST_GRACE_TICKS;
        return;
    }

    g_lineLostTicks  = 0U;
    g_lastCorrection = linePid_calculate(line.error);
    g_lastLeftCmd =
        clampInt16((int32_t)g_baseSpeed + g_lastCorrection, 0, PWM_MAX);
    g_lastRightCmd =
        clampInt16((int32_t)g_baseSpeed - g_lastCorrection, 0, PWM_MAX);

    driveSetLogical(g_lastLeftCmd, g_lastRightCmd);
}

static void UART0_printHelp(void)
{
    UART0_writeString(
        "Commands: A=seg auto  g=Step7 lap  G=line  F=arm white->black  f=fwd jog  v=rev jog  x=coast  b=brake  "
        "+/-=base  p/P=Kp  d/D=Kd\r\n");
    UART0_writeString(
        "Orient: w=swap motors  1=inv PWMA  2=inv PWMB  s=flip sensor  "
        "e=swap enc  3=inv encA  4=inv encB  r=reset enc  ?=help\r\n");
    UART0_writeString("Lap: [ target-1000  ] target+1000 ticks; PB22 fast blink means lap done\r\n");
    UART0_writeString("Gap: , bias-5  . bias+5; F stops after 3 confirmed black ticks\r\n");
}

static void step6_printParams(void)
{
    UART0_writeString("base=");
    UART0_writeDec32(g_baseSpeed);
    UART0_writeString(" Kp=");
    UART0_writeDec32(g_pidKp);
    UART0_writeString(" Ki=");
    UART0_writeDec32(g_pidKi);
    UART0_writeString(" Kd=");
    UART0_writeDec32(g_pidKd);
    UART0_writeString("\r\n");
}

static void step6_printOrientation(void)
{
    UART0_writeString("orient mSwap=");
    UART0_writeDec32(g_motorSwapSides);
    UART0_writeString(" invA=");
    UART0_writeDec32(g_motorInvertA);
    UART0_writeString(" invB=");
    UART0_writeDec32(g_motorInvertB);
    UART0_writeString(" sensorFlip=");
    UART0_writeDec32(g_sensorReverseLR);
    UART0_writeString(" eSwap=");
    UART0_writeDec32(g_encoderSwapSides);
    UART0_writeString(" eInvA=");
    UART0_writeDec32(g_encoderInvertA);
    UART0_writeString(" eInvB=");
    UART0_writeDec32(g_encoderInvertB);
    UART0_writeString("\r\n");
}

static void step6_stopForOrientationChange(void)
{
    g_driveMode = DRIVE_STANDBY;
    g_lapActive = 0U;
    g_lapDone   = 0U;
    step7_clearAutoRun();
    step7_clearGapTest();
    motorStop();
    linePid_reset();
}

static void step6_handleCommand(uint8_t cmd)
{
    if ((g_driveMode == DRIVE_AUTO) && (cmd != 'b') && (cmd != 'B') &&
        (cmd != '\r') && (cmd != '\n')) {
        UART0_writeString("\r\n[AUTO] command ignored; send b to brake\r\n");
        return;
    }

    switch (cmd) {
        case '\r':
        case '\n':
            break;
        case 'A':
        case 'a':
            step7_startAutoRun();
            break;
        case 'g':
            step7_startLapRun();
            break;
        case 'G':
            step7_clearAutoRun();
            step7_clearGapTest();
            g_lapActive = 0U;
            g_lapDone   = 0U;
            g_driveMode = DRIVE_RUN;
            linePid_reset();
            UART0_writeString("\r\n[RUN] continuous line PID armed\r\n");
            break;
        case 'f':
            step7_clearAutoRun();
            step7_clearGapTest();
            g_lapActive = 0U;
            g_lapDone   = 0U;
            g_driveMode = DRIVE_JOG_FWD;
            linePid_reset();
            UART0_writeString("\r\n[FWD] jog forward; send x to stop\r\n");
            break;
        case 'F':
            step7_startGapToBlackRun();
            break;
        case 'v':
        case 'V':
            step7_clearAutoRun();
            step7_clearGapTest();
            g_lapActive = 0U;
            g_lapDone   = 0U;
            g_driveMode = DRIVE_JOG_REV;
            linePid_reset();
            UART0_writeString("\r\n[REV] jog reverse; send x to stop\r\n");
            break;
        case 'x':
        case 'X':
            step7_clearAutoRun();
            step7_clearGapTest();
            g_lapActive = 0U;
            g_lapDone   = 0U;
            g_driveMode = DRIVE_STANDBY;
            motorStop();
            linePid_reset();
            UART0_writeString("\r\n[STBY] motors coast\r\n");
            break;
        case 'b':
        case 'B':
            step7_clearAutoRun();
            step7_clearGapTest();
            g_lapActive = 0U;
            g_lapDone   = 0U;
            g_driveMode = DRIVE_BRAKE;
            motorBrake();
            linePid_reset();
            UART0_writeString("\r\n[BRAKE] motors short-brake\r\n");
            break;
        case '+':
            g_baseSpeed =
                clampInt16((int32_t)g_baseSpeed + 20, STEP6_BASE_SPEED_MIN,
                           STEP6_BASE_SPEED_MAX);
            step6_printParams();
            break;
        case '-':
            g_baseSpeed =
                clampInt16((int32_t)g_baseSpeed - 20, STEP6_BASE_SPEED_MIN,
                           STEP6_BASE_SPEED_MAX);
            step6_printParams();
            break;
        case 'p':
            g_pidKp = clampInt16((int32_t)g_pidKp + 5, 0, 200);
            step6_printParams();
            break;
        case 'P':
            g_pidKp = clampInt16((int32_t)g_pidKp - 5, 0, 200);
            step6_printParams();
            break;
        case 'd':
            g_pidKd = clampInt16((int32_t)g_pidKd + 5, 0, 200);
            step6_printParams();
            break;
        case 'D':
            g_pidKd = clampInt16((int32_t)g_pidKd - 5, 0, 200);
            step6_printParams();
            break;
        case ']':
            if (g_lapTargetTicks <=
                (STEP7_LAP_TARGET_TICKS_MAX - STEP7_LAP_TARGET_STEP)) {
                g_lapTargetTicks += STEP7_LAP_TARGET_STEP;
            } else {
                g_lapTargetTicks = STEP7_LAP_TARGET_TICKS_MAX;
            }
            step7_printLapParams();
            break;
        case '[':
            if (g_lapTargetTicks >=
                (STEP7_LAP_TARGET_TICKS_MIN + STEP7_LAP_TARGET_STEP)) {
                g_lapTargetTicks -= STEP7_LAP_TARGET_STEP;
            } else {
                g_lapTargetTicks = STEP7_LAP_TARGET_TICKS_MIN;
            }
            step7_printLapParams();
            break;
        case '.':
            g_gapBalanceBias =
                clampInt16((int32_t)g_gapBalanceBias +
                               STEP7_GAP_BALANCE_BIAS_STEP,
                           STEP7_GAP_BALANCE_BIAS_MIN,
                           STEP7_GAP_BALANCE_BIAS_MAX);
            step7_printLapParams();
            break;
        case ',':
            g_gapBalanceBias =
                clampInt16((int32_t)g_gapBalanceBias -
                               STEP7_GAP_BALANCE_BIAS_STEP,
                           STEP7_GAP_BALANCE_BIAS_MIN,
                           STEP7_GAP_BALANCE_BIAS_MAX);
            step7_printLapParams();
            break;
        case 'w':
        case 'W':
            step6_stopForOrientationChange();
            g_motorSwapSides ^= 1U;
            step6_printOrientation();
            break;
        case '1':
            step6_stopForOrientationChange();
            g_motorInvertA ^= 1U;
            step6_printOrientation();
            break;
        case '2':
            step6_stopForOrientationChange();
            g_motorInvertB ^= 1U;
            step6_printOrientation();
            break;
        case 's':
        case 'S':
            step6_stopForOrientationChange();
            g_sensorReverseLR ^= 1U;
            step6_printOrientation();
            break;
        case 'e':
        case 'E':
            step6_stopForOrientationChange();
            g_encoderSwapSides ^= 1U;
            step6_printOrientation();
            break;
        case '3':
            step6_stopForOrientationChange();
            g_encoderInvertA ^= 1U;
            step6_printOrientation();
            break;
        case '4':
            step6_stopForOrientationChange();
            g_encoderInvertB ^= 1U;
            step6_printOrientation();
            break;
        case 'r':
        case 'R':
            step7_resetOdom();
            step7_clearAutoRun();
            step7_clearGapTest();
            UART0_writeString("\r\n[ENC] counters reset\r\n");
            step7_printLapParams();
            break;
        case '?':
            UART0_printHelp();
            step6_printParams();
            step6_printOrientation();
            step7_printLapParams();
            break;
        default:
            UART0_writeString("\r\n[?] unknown command, send ? for help\r\n");
            break;
    }
}

static void step6_printStatus(void)
{
    int32_t encLeft;
    int32_t encRight;

    step6_readLogicalEncoders(&encLeft, &encRight);
    g_lastOdomTicks = step7_getOdomTicks();

    UART0_writeString("mode=");
    UART0_writeString(driveModeName(g_driveMode));
    UART0_writeString(" line=0b");
    UART0_writeBinary8(g_lastLineRaw);
    UART0_writeString(" st=");
    UART0_writeString(lineStatusName(g_lastLineStatus));
    UART0_writeString(" n=");
    UART0_writeDec32(g_lastLineActive);
    UART0_writeString(" err=");
    UART0_writeDec32(g_lastLineError);
    UART0_writeString(" corr=");
    UART0_writeDec32(g_lastCorrection);
    UART0_writeString(" base=");
    UART0_writeDec32(g_baseSpeed);
    UART0_writeString(" L=");
    UART0_writeDec32(g_lastLeftCmd);
    UART0_writeString(" R=");
    UART0_writeDec32(g_lastRightCmd);
    UART0_writeString(" encL=");
    UART0_writeDec32(encLeft);
    UART0_writeString(" encR=");
    UART0_writeDec32(encRight);
    UART0_writeString(" odom=");
    UART0_writeDec32(g_lastOdomTicks);
    UART0_writeString(" tgt=");
    UART0_writeDec32(g_lapTargetTicks);
    UART0_writeString(" gap=");
    UART0_writeDec32(g_lineLostTicks);
    UART0_writeString(" lap=");
    if (g_lapDone != 0U) {
        UART0_writeString("DONE");
    } else if (g_lapActive != 0U) {
        UART0_writeString("RUN");
    } else {
        UART0_writeString("OFF");
    }
    UART0_writeString(" gapTest=");
    UART0_writeString(gapTestStateName(g_gapTestState));
    UART0_writeString(" gapOdom=");
    UART0_writeDec32(g_gapStopOdomTicks);
    UART0_writeString(" gapRaw=0b");
    UART0_writeBinary8(g_gapStopRaw);
    UART0_writeString(" gapBlk=");
    UART0_writeDec32(g_gapBlackTicks);
    UART0_writeString(" gapDL=");
    UART0_writeDec32(g_gapLastDeltaLeft);
    UART0_writeString(" gapDR=");
    UART0_writeDec32(g_gapLastDeltaRight);
    UART0_writeString(" gapBias=");
    UART0_writeDec32(g_gapBalanceBias);
    UART0_writeString(" auto=");
    if (g_autoSegment == AUTO_SEG_DONE) {
        UART0_writeString("DONE");
    } else if (g_autoSegment == AUTO_SEG_FAIL) {
        UART0_writeString("FAIL");
    } else if (g_autoSegment == AUTO_SEG_OFF) {
        UART0_writeString("OFF");
    } else if (autoSegmentIsRunSegment(g_autoSegment) != 0U) {
        UART0_writeString("RUN");
    } else {
        UART0_writeString("BAD");
    }
    UART0_writeString(" seg=");
    UART0_writeString(autoSegmentName(g_autoSegment));
    UART0_writeString(" segOdom=");
    UART0_writeDec32(g_autoSegOdomTicks);
    UART0_writeString(" autoReady=");
    UART0_writeDec32(g_autoStraightReady);
    UART0_writeString("\r\n");
}

/* ---- main ---- */
int main(void)
{
    SYSCFG_DL_init();
    motorStop();
    DL_TimerA_startCounter(PWM_MOTORS_INST);

    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);

    /* Enable encoder GPIOB interrupt */
    NVIC_ClearPendingIRQ(GPIO_ENCODER_INT_IRQN);
    NVIC_EnableIRQ(GPIO_ENCODER_INT_IRQN);

    /* Enable STBY (PB20 = high) */
    DL_GPIO_setPins(GPIO_MOTOR_STBY_PORT, GPIO_MOTOR_STBY_STBY_PIN);

    uint32_t statusElapsedMs = 0U;
    uint32_t pidElapsedMs    = 0U;
    uint32_t ledElapsedMs    = 0U;

    UART0_writeString("\r\n=== ER2024 Step7: One-lap Line PID ===\r\n");
    UART0_writeString("Build: ");
    UART0_writeString(ER2024_FW_BUILD_TAG);
    UART0_writeString("\r\n");
    UART0_writeString("PWM: PA8=HW-A/PWMA/AO/right PA9=HW-B/PWMB/BO/left 4kHz  Dir: PA16/21/22/23"
                      "  STBY:PB20\r\n");
    UART0_writeString("Enc: HW-A=PB17/18 HW-B=PB24/25; logical sides use orient flags\r\n");
    UART0_writeString("Line: black=1; sensorFlip decides logical left/right\r\n");
    UART0_printHelp();
    step6_printParams();
    step6_printOrientation();
    UART0_writeString("Boot mode is STBY. Send 'g' from A after wheels are safe.\r\n\r\n");

    while (1) {
        if (g_uartCommand != 0U) {
            uint8_t cmd = g_uartCommand;
            g_uartCommand = 0U;
            step6_handleCommand(cmd);
        }

        pidElapsedMs += 1U;
        if (pidElapsedMs >= LINE_PID_PERIOD_MS) {
            pidElapsedMs = 0U;
            step6_lineFollowUpdate();
        }

        statusElapsedMs += 1U;
        uint32_t statusPeriodMs =
            (g_driveMode == DRIVE_AUTO) ? STATUS_PRINT_AUTO_PERIOD_MS
                                        : STATUS_PRINT_PERIOD_MS;
        if (statusElapsedMs >= statusPeriodMs) {
            statusElapsedMs = 0U;
            step6_printStatus();
        }

        /* ---- LED toggle: fast blink after Step 7 lap stop ---- */
        uint32_t ledPeriodMs =
            (g_lapDone != 0U) ? LAP_DONE_LED_PERIOD_MS : LED_TOGGLE_PERIOD_MS;

        ledElapsedMs += 1U;
        if (ledElapsedMs >= ledPeriodMs) {
            ledElapsedMs = 0U;
            DL_GPIO_togglePins(GPIO_LED_PORT, GPIO_LED_LED1_PIN);
        }

        delay_ms(1U);
    }
}

/* ---- UART RX echo ---- */
void UART_0_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_0_INST)) {
        case DL_UART_MAIN_IIDX_RX: {
            uint8_t data = (uint8_t)DL_UART_Main_readData(UART_0_INST);
            g_uartCommand = data;
            break;
        }
        default:
            break;
    }
}

/* ---- Encoder GPIOB interrupt (GROUP1) ---- */
void GROUP1_IRQHandler(void)
{
    switch (DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1)) {
        case GPIO_ENCODER_INT_IIDX: {
            const uint32_t encoderIrqPins =
                GPIO_ENCODER_ENC1A_PIN | GPIO_ENCODER_ENC2A_PIN;
            uint32_t status =
                DL_GPIO_getEnabledInterruptStatus(GPIO_ENCODER_PORT,
                                                  encoderIrqPins);

            if ((status & GPIO_ENCODER_ENC1A_PIN) != 0U) {
                DL_GPIO_clearInterruptStatus(GPIO_ENCODER_PORT,
                                             GPIO_ENCODER_ENC1A_PIN);
                /* Raw HW-A sign comes from the old Step 5 channel check. */
                if ((DL_GPIO_readPins(GPIO_ENCODER_PORT,
                                      GPIO_ENCODER_ENC1B_PIN)) != 0U) {
                    g_encA--;
                } else {
                    g_encA++;
                }
            }

            if ((status & GPIO_ENCODER_ENC2A_PIN) != 0U) {
                DL_GPIO_clearInterruptStatus(GPIO_ENCODER_PORT,
                                             GPIO_ENCODER_ENC2A_PIN);
                if ((DL_GPIO_readPins(GPIO_ENCODER_PORT,
                                      GPIO_ENCODER_ENC2B_PIN)) != 0U) {
                    g_encB++;
                } else {
                    g_encB--;
                }
            }
            break;
        }
        default:
            break;
    }
}
