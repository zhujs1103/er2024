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
 * Serial output @ 115200-8N1:
 *   mode=RUN line=0bXXXXXXXX st=OK err=%d corr=%d base=%d L=%d R=%d encL=%d encR=%d odom=%d
 */

#include "ti_msp_dl_config.h"
#include "mpu6050_straight.h"

#include <stdint.h>

#ifndef DL_UART_Main_readData
#define DL_UART_Main_readData DL_UART_Main_receiveData
#endif

/* ---- timing ---- */
#define STATUS_PRINT_PERIOD_MS    (200U)
#define AUTO_STATUS_PRINT_PERIOD_MS (500U)
#define LINE_PID_PERIOD_MS        (20U)    /* 50 Hz line/PID control loop */
#define LED_TOGGLE_PERIOD_MS      (500U)
#define LAP_DONE_LED_PERIOD_MS    (100U)
#define UI_FAST_BLINK_PERIOD_MS   (100U)
#define BUTTON_START_DEBOUNCE_MS    (30U)
#define BUTTON_START_COUNTDOWN_MS   (3000U)
#define BUTTON_START_AUTO_DELAY_MS  (450U)
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

#define ER2024_FW_BUILD_TAG "2026-07-05-abcd-fresh-b21-auto-start-test"

#define UI_BUZZER_PORT            (GPIOA)
#define UI_BUZZER_PIN             (DL_GPIO_PIN_2)
#define UI_BUZZER_IOMUX           (IOMUX_PINCM7)
#define UI_LED_PORT               (GPIOA)
#define UI_LED_PIN                (DL_GPIO_PIN_14)
#define UI_LED_IOMUX              (IOMUX_PINCM36)
#define BUTTON_START_PORT         (GPIOB)
#define BUTTON_START_PIN          (DL_GPIO_PIN_21)
#define BUTTON_START_IOMUX        (IOMUX_PINCM49)

#define ABCD_WHITE_BLACK_MIN_TICKS     (1200)
#define ABCD_WHITE_BLACK_MAX_TICKS     (1900)
#define ABCD_WHITE_BLACK_CONFIRM_TICKS (1U)
#define ABCD_ENTRY_PROTECT_MIN_TICKS   (15)
#define ABCD_ENTRY_PROTECT_MAX_TICKS   (45)
#define ABCD_ACQUIRE_MAX_TICKS         (45)
#define ABCD_CENTER_CONFIRM_TICKS      (2U)
#define ABCD_CENTER_MASK               ((uint8_t)0x3CU)
#define ABCD_ARC_ENTRY_GRACE_TICKS     (120)
#define ABCD_ARC_EXIT_MIN_TICKS        (1500)
#define ABCD_ARC_EXIT_MAX_TICKS        (2600)
#define ABCD_ARC_LOST_CONFIRM_TICKS    (3U)
#define ABCD_ARC_YAW_TOL_DEG100        (1000L)
#define ABCD_ALIGN_YAW_TOL_DEG100      (500L)
#define ABCD_ALIGN_CONFIRM_TICKS       (2U)
#define ABCD_ALIGN_MAX_TICKS           (250)
#define ABCD_AUTO_YAW_LIMIT_DEG100     (45000L)
#define ABCD_YAW_TARGET_AB_DEG100      (0L)
#define ABCD_YAW_TARGET_CD_DEG100      (-15300L)
#define ABCD_YAW_TARGET_DONE_DEG100    (-36000L)

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
    DRIVE_IMU_STRAIGHT,
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
    ABCD_STATE_OFF = 0,
    ABCD_STATE_AB_STRAIGHT,
    ABCD_STATE_B_GUARD,
    ABCD_STATE_B_ACQUIRE,
    ABCD_STATE_BC_LINE,
    ABCD_STATE_C_ALIGN,
    ABCD_STATE_CD_STRAIGHT,
    ABCD_STATE_D_GUARD,
    ABCD_STATE_D_ACQUIRE,
    ABCD_STATE_DA_LINE,
    ABCD_STATE_DONE,
    ABCD_STATE_FAIL
} abcd_state_t;

typedef enum {
    UI_FEEDBACK_NONE = 0,
    UI_FEEDBACK_BOOT,
    UI_FEEDBACK_AUTO_START,
    UI_FEEDBACK_CAL_OK,
    UI_FEEDBACK_DONE,
    UI_FEEDBACK_FAIL
} ui_feedback_pattern_t;

typedef enum {
    BUTTON_START_IDLE = 0,
    BUTTON_START_COUNTDOWN,
    BUTTON_START_AUTO_PENDING
} button_start_state_t;

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
static int16_t       g_lastStraightCorrection = 0;
static int16_t       g_lastImuCorrection = 0;
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
static int32_t       g_gapStartLeftTicks = 0;
static int32_t       g_gapStartRightTicks = 0;
static int32_t       g_gapPrevLeftTicks  = 0;
static int32_t       g_gapPrevRightTicks = 0;
static int32_t       g_gapLastDeltaLeft  = 0;
static int32_t       g_gapLastDeltaRight = 0;
static gap_test_state_t g_gapTestState = GAP_TEST_OFF;
static abcd_state_t  g_abcdState = ABCD_STATE_OFF;
static int32_t       g_abcdStateStartOdomTicks = 0;
static int32_t       g_abcdStateOdomTicks = 0;
static uint8_t       g_abcdLostTicks = 0U;
static uint8_t       g_abcdCenterTicks = 0U;
static uint8_t       g_abcdPointCount = 0U;
static uint8_t       g_abcdBlackTicks = 0U;
static uint8_t       g_abcdYawTicks = 0U;
static uint8_t       g_abcdWhiteReady = 0U;
static int32_t       g_abcdYawDeg100 = 0;
static int32_t       g_abcdYawTargetDeg100 = 0;
static int32_t       g_abcdYawErrorDeg100 = 0;
static int32_t       g_abcdHoldYawDeg100 = 0;
static ui_feedback_pattern_t g_uiFeedbackPattern = UI_FEEDBACK_NONE;
static uint16_t      g_uiFeedbackElapsedMs = 0U;
static uint16_t      g_uiBlinkElapsedMs = 0U;
static uint8_t       g_uiFastBlink = 0U;
static uint8_t       g_uiBlinkOn = 0U;
static button_start_state_t g_buttonStartState = BUTTON_START_IDLE;
static uint32_t      g_buttonStartElapsedMs = 0U;
static uint8_t       g_buttonRawPressed = 0U;
static uint8_t       g_buttonStablePressed = 0U;
static uint16_t      g_buttonDebounceMs = 0U;
static uint8_t       g_buttonCountdownSecond = 0U;

static uint8_t g_motorSwapSides   = STEP6_SWAP_MOTOR_SIDES_DEFAULT;
static uint8_t g_motorInvertA     = STEP6_INVERT_MOTOR_A_DEFAULT;
static uint8_t g_motorInvertB     = STEP6_INVERT_MOTOR_B_DEFAULT;
static uint8_t g_sensorReverseLR  = STEP6_REVERSE_SENSOR_LR_DEFAULT;
static uint8_t g_encoderSwapSides = STEP6_SWAP_ENCODER_SIDES_DEFAULT;
static uint8_t g_encoderInvertA   = STEP6_INVERT_ENCODER_A_DEFAULT;
static uint8_t g_encoderInvertB   = STEP6_INVERT_ENCODER_B_DEFAULT;

static void linePid_reset(void);
static int16_t clampInt16(int32_t value, int16_t minVal, int16_t maxVal);
static void abcd_clearAutoRun(void);
static void abcd_brake(abcd_state_t state, const char *reason);
static void abcd_startAutoRun(void);
static void imuPrintState(void);
static void uiFeedback_clear(void);
static void uiFeedback_playBoot(void);
static void uiFeedback_playAutoStart(void);
static void uiFeedback_playCalOk(void);
static void uiFeedback_playDone(void);
static void uiFeedback_playFail(void);
static void buttonStart_init(void);
static void buttonStart_update1ms(void);
static mpu6050_straight_init_result_t imuRunStillCalibrationCore(void);

static void uiFeedback_setOutputs(uint8_t buzzerOn, uint8_t ledOn)
{
    if (buzzerOn != 0U) {
        DL_GPIO_clearPins(UI_BUZZER_PORT, UI_BUZZER_PIN);
    } else {
        DL_GPIO_setPins(UI_BUZZER_PORT, UI_BUZZER_PIN);
    }

    if (ledOn != 0U) {
        DL_GPIO_setPins(UI_LED_PORT, UI_LED_PIN);
    } else {
        DL_GPIO_clearPins(UI_LED_PORT, UI_LED_PIN);
    }
}

static void uiFeedback_init(void)
{
    DL_GPIO_initDigitalOutputFeatures(UI_BUZZER_IOMUX,
                                      DL_GPIO_INVERSION_DISABLE,
                                      DL_GPIO_RESISTOR_PULL_UP,
                                      DL_GPIO_DRIVE_STRENGTH_LOW,
                                      DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(UI_LED_IOMUX,
                                      DL_GPIO_INVERSION_DISABLE,
                                      DL_GPIO_RESISTOR_PULL_DOWN,
                                      DL_GPIO_DRIVE_STRENGTH_LOW,
                                      DL_GPIO_HIZ_DISABLE);

    DL_GPIO_setPins(UI_BUZZER_PORT, UI_BUZZER_PIN);
    DL_GPIO_clearPins(UI_LED_PORT, UI_LED_PIN);
    DL_GPIO_enableOutput(UI_BUZZER_PORT, UI_BUZZER_PIN);
    DL_GPIO_enableOutput(UI_LED_PORT, UI_LED_PIN);

    uiFeedback_clear();
}

static void uiFeedback_start(ui_feedback_pattern_t pattern)
{
    g_uiFeedbackPattern = pattern;
    g_uiFeedbackElapsedMs = 0U;
    g_uiBlinkElapsedMs = 0U;
    g_uiFastBlink = 0U;
    g_uiBlinkOn = 0U;
    uiFeedback_setOutputs(0U, 0U);
}

static void uiFeedback_clear(void)
{
    g_uiFeedbackPattern = UI_FEEDBACK_NONE;
    g_uiFeedbackElapsedMs = 0U;
    g_uiBlinkElapsedMs = 0U;
    g_uiFastBlink = 0U;
    g_uiBlinkOn = 0U;
    uiFeedback_setOutputs(0U, 0U);
}

static void uiFeedback_playBoot(void)
{
    uiFeedback_start(UI_FEEDBACK_BOOT);
}

static void uiFeedback_playAutoStart(void)
{
    uiFeedback_start(UI_FEEDBACK_AUTO_START);
}

static void uiFeedback_playCalOk(void)
{
    uiFeedback_start(UI_FEEDBACK_CAL_OK);
}

static void uiFeedback_playDone(void)
{
    uiFeedback_start(UI_FEEDBACK_DONE);
}

static void uiFeedback_playFail(void)
{
    uiFeedback_start(UI_FEEDBACK_FAIL);
}

static void uiFeedback_finish(uint8_t fastBlink)
{
    g_uiFeedbackPattern = UI_FEEDBACK_NONE;
    g_uiFeedbackElapsedMs = 0U;
    g_uiBlinkElapsedMs = 0U;
    g_uiFastBlink = fastBlink;
    g_uiBlinkOn = 0U;
    uiFeedback_setOutputs(0U, 0U);
}

static void uiFeedback_updateFastBlink(void)
{
    if (g_uiFastBlink == 0U) {
        uiFeedback_setOutputs(0U, 0U);
        return;
    }

    g_uiBlinkElapsedMs++;
    if (g_uiBlinkElapsedMs >= UI_FAST_BLINK_PERIOD_MS) {
        g_uiBlinkElapsedMs = 0U;
        g_uiBlinkOn ^= 1U;
    }

    uiFeedback_setOutputs(0U, g_uiBlinkOn);
}

static void uiFeedback_update1ms(void)
{
    uint8_t active = 0U;

    switch (g_uiFeedbackPattern) {
        case UI_FEEDBACK_BOOT:
        case UI_FEEDBACK_AUTO_START:
            active = (g_uiFeedbackElapsedMs < 80U) ? 1U : 0U;
            uiFeedback_setOutputs(active, active);
            g_uiFeedbackElapsedMs++;
            if (g_uiFeedbackElapsedMs >= 120U) {
                uiFeedback_finish(0U);
            }
            break;

        case UI_FEEDBACK_CAL_OK:
            active = (((g_uiFeedbackElapsedMs < 80U) ||
                       ((g_uiFeedbackElapsedMs >= 200U) &&
                        (g_uiFeedbackElapsedMs < 280U)))) ? 1U : 0U;
            uiFeedback_setOutputs(active, active);
            g_uiFeedbackElapsedMs++;
            if (g_uiFeedbackElapsedMs >= 420U) {
                uiFeedback_finish(0U);
            }
            break;

        case UI_FEEDBACK_DONE:
            active = (g_uiFeedbackElapsedMs < 350U) ? 1U : 0U;
            uiFeedback_setOutputs(active, active);
            g_uiFeedbackElapsedMs++;
            if (g_uiFeedbackElapsedMs >= 450U) {
                uiFeedback_finish(1U);
            }
            break;

        case UI_FEEDBACK_FAIL:
            active = ((g_uiFeedbackElapsedMs < 600U) &&
                      ((g_uiFeedbackElapsedMs % 200U) < 80U)) ? 1U : 0U;
            uiFeedback_setOutputs(active, active);
            g_uiFeedbackElapsedMs++;
            if (g_uiFeedbackElapsedMs >= 650U) {
                uiFeedback_finish(1U);
            }
            break;

        case UI_FEEDBACK_NONE:
        default:
            uiFeedback_updateFastBlink();
            break;
    }
}

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
        case DRIVE_IMU_STRAIGHT:
            return "IMU";
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

static const char *abcdStateName(abcd_state_t state)
{
    switch (state) {
        case ABCD_STATE_AB_STRAIGHT:
            return "AB_STRAIGHT";
        case ABCD_STATE_B_GUARD:
            return "B_GUARD";
        case ABCD_STATE_B_ACQUIRE:
            return "B_ACQUIRE";
        case ABCD_STATE_BC_LINE:
            return "BC_LINE";
        case ABCD_STATE_C_ALIGN:
            return "C_ALIGN";
        case ABCD_STATE_CD_STRAIGHT:
            return "CD_STRAIGHT";
        case ABCD_STATE_D_GUARD:
            return "D_GUARD";
        case ABCD_STATE_D_ACQUIRE:
            return "D_ACQUIRE";
        case ABCD_STATE_DA_LINE:
            return "DA_LINE";
        case ABCD_STATE_DONE:
            return "DONE";
        case ABCD_STATE_FAIL:
            return "FAIL";
        case ABCD_STATE_OFF:
        default:
            return "OFF";
    }
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
    int32_t relLeft;
    int32_t relRight;

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

    relLeft = left - g_gapStartLeftTicks;
    relRight = right - g_gapStartRightTicks;
    if (relLeft < 0) {
        relLeft = 0;
    }
    if (relRight < 0) {
        relRight = 0;
    }

    positionError = relLeft - relRight;
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

    g_gapStartLeftTicks = left;
    g_gapStartRightTicks = right;
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

static void abcd_clearAutoRun(void)
{
    g_abcdState = ABCD_STATE_OFF;
    g_abcdStateStartOdomTicks = 0;
    g_abcdStateOdomTicks = 0;
    g_abcdLostTicks = 0U;
    g_abcdCenterTicks = 0U;
    g_abcdPointCount = 0U;
    g_abcdBlackTicks = 0U;
    g_abcdYawTicks = 0U;
    g_abcdWhiteReady = 0U;
    g_abcdYawDeg100 = 0;
    g_abcdYawTargetDeg100 = 0;
    g_abcdYawErrorDeg100 = 0;
    g_abcdHoldYawDeg100 = 0;
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
    UART0_writeString(abcdStateName(g_abcdState));
    UART0_writeString(" autoOdom=");
    UART0_writeDec32(g_abcdStateOdomTicks);
    UART0_writeString(" point=");
    UART0_writeDec32(g_abcdPointCount);
    UART0_writeString("\r\n");
}

static void step7_startLapRun(void)
{
    uiFeedback_clear();
    abcd_clearAutoRun();
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
    uiFeedback_clear();
    abcd_clearAutoRun();
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
    uiFeedback_playDone();

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
    g_lastStraightCorrection = 0;
    g_lastImuCorrection = 0;
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
    g_lastStraightCorrection = step7_gapStraightCorrection();
    g_lastImuCorrection = 0;
    g_lastCorrection = g_lastStraightCorrection;
    g_lastLeftCmd =
        clampInt16((int32_t)g_baseSpeed - g_lastCorrection, 0, PWM_MAX);
    g_lastRightCmd =
        clampInt16((int32_t)g_baseSpeed + g_lastCorrection, 0, PWM_MAX);
    driveSetLogical(g_lastLeftCmd, g_lastRightCmd);
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

static uint8_t imuDriveStraight(void)
{
    const mpu6050_straight_state_t *mpu;
    int32_t combinedCorrection;

    g_lastStraightCorrection = step7_gapStraightCorrection();
    g_lastImuCorrection = mpu6050_straight_update(LINE_PID_PERIOD_MS);
    mpu = mpu6050_straight_state();

    if (mpu->ready == 0U) {
        g_lastLeftCmd = 0;
        g_lastRightCmd = 0;
        g_driveMode = DRIVE_BRAKE;
        motorBrake();
        UART0_writeString("\r\n[IMU] read failed; braking\r\n");
        return 1U;
    }

    /*
     * Bench check: yawRate100 > 0 means the car nose is turning left.
     * Encoder correction uses the F-mode sign; IMU correction has the opposite
     * drive sign, so subtract it before applying the usual straight formula.
     */
    combinedCorrection =
        (int32_t)g_lastStraightCorrection - (int32_t)g_lastImuCorrection;
    g_lastCorrection = clampInt16(combinedCorrection,
                                  -STEP7_GAP_BALANCE_MAX,
                                  STEP7_GAP_BALANCE_MAX);
    g_lastLeftCmd =
        clampInt16((int32_t)g_baseSpeed - g_lastCorrection, 0, PWM_MAX);
    g_lastRightCmd =
        clampInt16((int32_t)g_baseSpeed + g_lastCorrection, 0, PWM_MAX);
    driveSetLogical(g_lastLeftCmd, g_lastRightCmd);

    return 0U;
}

static uint8_t step7_gapToBlackUpdate(const line_position_t *line)
{
    g_lastOdomTicks = step7_getOdomTicks();
    g_lastCorrection = 0;

    if (g_gapTestState == GAP_TEST_WAIT_WHITE) {
        g_lastLeftCmd  = 0;
        g_lastRightCmd = 0;
        motorStop();

        if (line->activeCount == 0U) {
            step7_resetOdom();
            if (g_driveMode == DRIVE_AUTO) {
                g_abcdStateStartOdomTicks = 0;
                g_abcdStateOdomTicks = 0;
            }
            step7_gapResetStraightController();
            g_gapTestState = GAP_TEST_RUNNING;
            g_gapBlackTicks = 0U;
            UART0_writeString("\r\n[GAP] white detected; odom reset and run\r\n");
        }
        return 0U;
    }

    if (g_gapTestState == GAP_TEST_DONE) {
        motorBrake();
        g_lastLeftCmd  = 0;
        g_lastRightCmd = 0;
        return 0U;
    }

    if (g_gapTestState == GAP_TEST_RUNNING) {
        if (line->activeCount == 0U) {
            g_gapBlackTicks = 0U;
            step7_gapDriveStraight();
            return 0U;
        }

        if (g_gapBlackTicks < STEP7_GAP_BLACK_CONFIRM_TICKS) {
            g_gapBlackTicks++;
        }

        if (g_gapBlackTicks < STEP7_GAP_BLACK_CONFIRM_TICKS) {
            step7_gapDriveStraight();
            return 0U;
        }
    }

    g_gapTestState = GAP_TEST_DONE;
    g_gapStopOdomTicks = g_lastOdomTicks;
    g_gapStopRaw = line->raw;
    return 1U;
}

static void abcd_printEvent(const char *event)
{
    UART0_writeString("\r\n[AUTO] ");
    UART0_writeString(event);
    UART0_writeString(" state=");
    UART0_writeString(abcdStateName(g_abcdState));
    UART0_writeString(" odom=");
    UART0_writeDec32(g_lastOdomTicks);
    UART0_writeString(" stateOdom=");
    UART0_writeDec32(g_abcdStateOdomTicks);
    UART0_writeString(" point=");
    UART0_writeDec32(g_abcdPointCount);
    UART0_writeString(" yaw=");
    UART0_writeDec32(g_abcdYawDeg100);
    UART0_writeString(" tgt=");
    UART0_writeDec32(g_abcdYawTargetDeg100);
    UART0_writeString(" err=");
    UART0_writeDec32(g_abcdYawErrorDeg100);
    UART0_writeString(" hold=");
    UART0_writeDec32(g_abcdHoldYawDeg100);
    UART0_writeString("\r\n");
}

static int32_t abcd_abs32(int32_t value)
{
    return (value < 0) ? -value : value;
}

static int32_t abcd_clampInt32(int32_t value, int32_t minVal, int32_t maxVal)
{
    if (value < minVal) {
        return minVal;
    }
    if (value > maxVal) {
        return maxVal;
    }
    return value;
}

static void abcd_setYawTarget(int32_t targetDeg100)
{
    g_abcdYawTargetDeg100 = targetDeg100;
    g_abcdYawErrorDeg100 = g_abcdYawDeg100 - g_abcdYawTargetDeg100;
}

static uint8_t abcd_updateYaw(void)
{
    const mpu6050_straight_state_t *mpu;

    (void)mpu6050_straight_update(LINE_PID_PERIOD_MS);
    mpu = mpu6050_straight_state();

    if (mpu->ready == 0U) {
        g_abcdYawErrorDeg100 = g_abcdYawDeg100 - g_abcdYawTargetDeg100;
        return 0U;
    }

    g_abcdYawDeg100 +=
        (mpu->yawRateDps100 * (int32_t)LINE_PID_PERIOD_MS) / 1000L;
    g_abcdYawDeg100 = abcd_clampInt32(g_abcdYawDeg100,
                                      -ABCD_AUTO_YAW_LIMIT_DEG100,
                                      ABCD_AUTO_YAW_LIMIT_DEG100);
    g_abcdYawErrorDeg100 = g_abcdYawDeg100 - g_abcdYawTargetDeg100;

    return 1U;
}

static uint8_t abcd_yawWithin(int32_t targetDeg100, int32_t toleranceDeg100)
{
    abcd_setYawTarget(targetDeg100);
    return (abcd_abs32(g_abcdYawErrorDeg100) <= toleranceDeg100) ? 1U : 0U;
}

static void abcd_applyYawStraightFromCurrentYaw(int32_t targetDeg100)
{
    int32_t combinedCorrection;

    abcd_setYawTarget(targetDeg100);
    g_lastStraightCorrection = step7_gapStraightCorrection();
    g_lastImuCorrection =
        mpu6050_straight_correction_from_error(g_abcdYawErrorDeg100);
    combinedCorrection =
        (int32_t)g_lastStraightCorrection - (int32_t)g_lastImuCorrection;

    g_lastCorrection = clampInt16(combinedCorrection,
                                  -STEP7_GAP_BALANCE_MAX,
                                  STEP7_GAP_BALANCE_MAX);
    g_lastLeftCmd =
        clampInt16((int32_t)g_baseSpeed - g_lastCorrection, 0, PWM_MAX);
    g_lastRightCmd =
        clampInt16((int32_t)g_baseSpeed + g_lastCorrection, 0, PWM_MAX);
    driveSetLogical(g_lastLeftCmd, g_lastRightCmd);
}

static uint8_t abcd_driveYawStraight(int32_t targetDeg100)
{
    abcd_setYawTarget(targetDeg100);
    if (abcd_updateYaw() == 0U) {
        abcd_brake(ABCD_STATE_FAIL, "imu lost");
        return 1U;
    }

    abcd_applyYawStraightFromCurrentYaw(targetDeg100);
    return 0U;
}

static void abcd_enterState(abcd_state_t state)
{
    g_abcdState = state;
    g_abcdStateStartOdomTicks = step7_getOdomTicks();
    g_abcdStateOdomTicks = 0;
    g_abcdLostTicks = 0U;
    g_abcdCenterTicks = 0U;
    g_abcdBlackTicks = 0U;
    g_abcdYawTicks = 0U;
    g_lineLostTicks = 0U;
    g_gapTestState = GAP_TEST_OFF;
    g_gapStopOdomTicks = 0;
    g_gapStopRaw = 0U;
    g_gapBlackTicks = 0U;

    switch (state) {
        case ABCD_STATE_AB_STRAIGHT:
        case ABCD_STATE_B_GUARD:
        case ABCD_STATE_B_ACQUIRE:
            abcd_setYawTarget(ABCD_YAW_TARGET_AB_DEG100);
            break;
        case ABCD_STATE_BC_LINE:
            abcd_setYawTarget(ABCD_YAW_TARGET_CD_DEG100);
            g_abcdHoldYawDeg100 = g_abcdYawDeg100;
            break;
        case ABCD_STATE_C_ALIGN:
        case ABCD_STATE_CD_STRAIGHT:
        case ABCD_STATE_D_GUARD:
        case ABCD_STATE_D_ACQUIRE:
            abcd_setYawTarget(ABCD_YAW_TARGET_CD_DEG100);
            break;
        case ABCD_STATE_DA_LINE:
        case ABCD_STATE_DONE:
            abcd_setYawTarget(ABCD_YAW_TARGET_DONE_DEG100);
            g_abcdHoldYawDeg100 = g_abcdYawDeg100;
            break;
        case ABCD_STATE_FAIL:
        case ABCD_STATE_OFF:
        default:
            break;
    }

    g_abcdWhiteReady = (state == ABCD_STATE_AB_STRAIGHT) ? 0U : 1U;
    step7_gapResetStraightController();
    linePid_reset();
}

static void abcd_brake(abcd_state_t state, const char *reason)
{
    g_lastOdomTicks = step7_getOdomTicks();
    g_driveMode = DRIVE_BRAKE;
    g_abcdState = state;
    g_lapActive = 0U;
    g_lapDone = (state == ABCD_STATE_DONE) ? 1U : 0U;
    g_lastLeftCmd = 0;
    g_lastRightCmd = 0;
    motorBrake();
    linePid_reset();
    g_gapTestState = GAP_TEST_OFF;
    if (state == ABCD_STATE_DONE) {
        uiFeedback_playDone();
    } else {
        uiFeedback_playFail();
    }

    UART0_writeString("\r\n[AUTO] ");
    UART0_writeString(reason);
    UART0_writeString(" state=");
    UART0_writeString(abcdStateName(g_abcdState));
    UART0_writeString(" odom=");
    UART0_writeDec32(g_lastOdomTicks);
    UART0_writeString(" stateOdom=");
    UART0_writeDec32(g_abcdStateOdomTicks);
    UART0_writeString(" yaw=");
    UART0_writeDec32(g_abcdYawDeg100);
    UART0_writeString(" tgt=");
    UART0_writeDec32(g_abcdYawTargetDeg100);
    UART0_writeString(" err=");
    UART0_writeDec32(g_abcdYawErrorDeg100);
    UART0_writeString(" hold=");
    UART0_writeDec32(g_abcdHoldYawDeg100);
    UART0_writeString("\r\n");
}

static uint8_t abcd_lineNearCenter(const line_position_t *line)
{
    if (line->status == LINE_STATUS_LOST) {
        return 0U;
    }
    if (line->status == LINE_STATUS_ALL_BLACK) {
        return 1U;
    }
    return ((line->raw & ABCD_CENTER_MASK) != 0U) ? 1U : 0U;
}

static uint8_t abcd_updateWhiteStraight(const line_position_t *line,
                                        int32_t yawTargetDeg100,
                                        abcd_state_t nextState)
{
    if (g_abcdWhiteReady == 0U) {
        g_lastLeftCmd = 0;
        g_lastRightCmd = 0;
        motorStop();

        if (line->activeCount == 0U) {
            g_abcdWhiteReady = 1U;
            g_abcdStateStartOdomTicks = step7_getOdomTicks();
            g_abcdStateOdomTicks = 0;
            g_abcdBlackTicks = 0U;
            step7_gapResetStraightController();
            abcd_setYawTarget(yawTargetDeg100);
        }
        return 0U;
    }

    if ((g_abcdStateOdomTicks >= ABCD_WHITE_BLACK_MIN_TICKS) &&
        (line->activeCount > 0U)) {
        if (g_abcdBlackTicks < ABCD_WHITE_BLACK_CONFIRM_TICKS) {
            g_abcdBlackTicks++;
        }
    } else {
        g_abcdBlackTicks = 0U;
    }

    if (g_abcdBlackTicks >= ABCD_WHITE_BLACK_CONFIRM_TICKS) {
        abcd_enterState(nextState);
        if (abcd_driveYawStraight(yawTargetDeg100) != 0U) {
            return 0U;
        }
        return 1U;
    }

    if (g_abcdStateOdomTicks > ABCD_WHITE_BLACK_MAX_TICKS) {
        abcd_brake(ABCD_STATE_FAIL, "white missed black");
        return 0U;
    }

    (void)abcd_driveYawStraight(yawTargetDeg100);
    return 0U;
}

static uint8_t abcd_updateGuard(const line_position_t *line,
                                int32_t yawTargetDeg100,
                                abcd_state_t nextState)
{
    if (g_abcdStateOdomTicks < ABCD_ENTRY_PROTECT_MIN_TICKS) {
        return abcd_driveYawStraight(yawTargetDeg100);
    }

    if (abcd_lineNearCenter(line) != 0U) {
        abcd_enterState(nextState);
        step7_driveLinePid(line);
        return 1U;
    }

    if (g_abcdStateOdomTicks >= ABCD_ENTRY_PROTECT_MAX_TICKS) {
        if (line->activeCount > 0U) {
            abcd_enterState(nextState);
            step7_driveLinePid(line);
            return 1U;
        }
        abcd_brake(ABCD_STATE_FAIL, "entry lost black");
        return 1U;
    }

    return abcd_driveYawStraight(yawTargetDeg100);
}

static uint8_t abcd_updateAcquire(const line_position_t *line,
                                  int32_t yawTargetDeg100,
                                  abcd_state_t nextState)
{
    if (abcd_lineNearCenter(line) != 0U) {
        if (g_abcdCenterTicks < ABCD_CENTER_CONFIRM_TICKS) {
            g_abcdCenterTicks++;
        }
    } else {
        g_abcdCenterTicks = 0U;
    }

    if (g_abcdCenterTicks >= ABCD_CENTER_CONFIRM_TICKS) {
        abcd_enterState(nextState);
        step7_driveLinePid(line);
        return 1U;
    }

    if (g_abcdStateOdomTicks >= ABCD_ACQUIRE_MAX_TICKS) {
        abcd_brake(ABCD_STATE_FAIL, "center not acquired");
        return 1U;
    }

    return abcd_driveYawStraight(yawTargetDeg100);
}

static uint8_t abcd_updateArcLine(const line_position_t *line,
                                  int32_t exitYawTargetDeg100)
{
    abcd_setYawTarget(exitYawTargetDeg100);
    if (abcd_updateYaw() == 0U) {
        abcd_brake(ABCD_STATE_FAIL, "imu lost");
        return 0U;
    }
    g_lastImuCorrection =
        mpu6050_straight_correction_from_error(g_abcdYawErrorDeg100);

    if (line->status == LINE_STATUS_LOST) {
        if (g_abcdStateOdomTicks < ABCD_ARC_ENTRY_GRACE_TICKS) {
            g_abcdLostTicks = 0U;
            step7_driveWithLastLineCorrection();
            return 0U;
        }

        if (g_abcdStateOdomTicks >= ABCD_ARC_EXIT_MIN_TICKS) {
            if (g_abcdLostTicks < ABCD_ARC_LOST_CONFIRM_TICKS) {
                g_abcdLostTicks++;
            }
            if (g_abcdLostTicks >= ABCD_ARC_LOST_CONFIRM_TICKS) {
                return 1U;
            }
            abcd_applyYawStraightFromCurrentYaw(g_abcdHoldYawDeg100);
            return 0U;
        }

        if (g_lineLostTicks < STEP7_LOST_GRACE_TICKS) {
            g_lineLostTicks++;
            step7_driveWithLastLineCorrection();
            return 0U;
        }

        abcd_brake(ABCD_STATE_FAIL, "arc lost early");
        return 0U;
    }

    g_abcdLostTicks = 0U;
    g_abcdYawTicks = 0U;
    g_lineLostTicks = 0U;

    if (g_abcdStateOdomTicks > ABCD_ARC_EXIT_MAX_TICKS) {
        abcd_brake(ABCD_STATE_FAIL, "arc exit missed");
        return 0U;
    }

    if (line->status == LINE_STATUS_ALL_BLACK) {
        step7_driveLinePid(line);
        return 0U;
    }

    g_abcdHoldYawDeg100 = g_abcdYawDeg100;
    step7_driveLinePid(line);
    return 0U;
}

static uint8_t abcd_updateAlign(int32_t yawTargetDeg100,
                                abcd_state_t nextState)
{
    if (abcd_driveYawStraight(yawTargetDeg100) != 0U) {
        return 1U;
    }

    if (abcd_yawWithin(yawTargetDeg100, ABCD_ALIGN_YAW_TOL_DEG100) != 0U) {
        if (g_abcdYawTicks < ABCD_ALIGN_CONFIRM_TICKS) {
            g_abcdYawTicks++;
        }
    } else {
        g_abcdYawTicks = 0U;
    }

    if (g_abcdYawTicks >= ABCD_ALIGN_CONFIRM_TICKS) {
        abcd_enterState(nextState);
        return 1U;
    }

    if (g_abcdStateOdomTicks > ABCD_ALIGN_MAX_TICKS) {
        abcd_brake(ABCD_STATE_FAIL, "align timeout");
        return 1U;
    }

    return 0U;
}

static void abcd_startAutoRun(void)
{
    const mpu6050_straight_state_t *mpu = mpu6050_straight_state();

    uiFeedback_clear();
    step7_clearGapTest();
    g_lapActive = 0U;
    g_lapDone = 0U;
    motorStop();
    abcd_clearAutoRun();
    linePid_reset();

    if (mpu->ready == 0U) {
        g_driveMode = DRIVE_STANDBY;
        uiFeedback_playFail();
        UART0_writeString("\r\n[AUTO] IMU not ready; send uppercase I first\r\n");
        imuPrintState();
        return;
    }

    step7_resetOdom();
    step7_gapResetStraightController();
    mpu6050_straight_reset();
    g_abcdYawDeg100 = ABCD_YAW_TARGET_AB_DEG100;
    abcd_setYawTarget(ABCD_YAW_TARGET_AB_DEG100);
    g_driveMode = DRIVE_AUTO;

    UART0_writeString("\r\n[AUTO] ABCD fresh run armed; IMU yaw targets enabled\r\n");
    abcd_enterState(ABCD_STATE_AB_STRAIGHT);
    uiFeedback_playAutoStart();
    abcd_printEvent("start");
}

static void abcd_update(const line_position_t *line)
{
    g_lastOdomTicks = step7_getOdomTicks();
    g_abcdStateOdomTicks = g_lastOdomTicks - g_abcdStateStartOdomTicks;
    if (g_abcdStateOdomTicks < 0) {
        g_abcdStateOdomTicks = 0;
    }

    switch (g_abcdState) {
        case ABCD_STATE_AB_STRAIGHT:
            if (abcd_updateWhiteStraight(line,
                                         ABCD_YAW_TARGET_AB_DEG100,
                                         ABCD_STATE_B_GUARD) != 0U) {
                g_abcdPointCount = 1U;
            }
            break;

        case ABCD_STATE_B_GUARD:
            (void)abcd_updateGuard(line,
                                   ABCD_YAW_TARGET_AB_DEG100,
                                   ABCD_STATE_BC_LINE);
            break;

        case ABCD_STATE_B_ACQUIRE:
            (void)abcd_updateAcquire(line,
                                     ABCD_YAW_TARGET_AB_DEG100,
                                     ABCD_STATE_BC_LINE);
            break;

        case ABCD_STATE_BC_LINE:
            if (abcd_updateArcLine(line, ABCD_YAW_TARGET_CD_DEG100) != 0U) {
                g_abcdPointCount = 2U;
                abcd_enterState(ABCD_STATE_C_ALIGN);
                (void)abcd_driveYawStraight(ABCD_YAW_TARGET_CD_DEG100);
            }
            break;

        case ABCD_STATE_C_ALIGN:
            (void)abcd_updateAlign(ABCD_YAW_TARGET_CD_DEG100,
                                   ABCD_STATE_CD_STRAIGHT);
            break;

        case ABCD_STATE_CD_STRAIGHT:
            if (abcd_updateWhiteStraight(line,
                                         ABCD_YAW_TARGET_CD_DEG100,
                                         ABCD_STATE_D_GUARD) != 0U) {
                g_abcdPointCount = 3U;
            }
            break;

        case ABCD_STATE_D_GUARD:
            (void)abcd_updateGuard(line,
                                   ABCD_YAW_TARGET_CD_DEG100,
                                   ABCD_STATE_DA_LINE);
            break;

        case ABCD_STATE_D_ACQUIRE:
            (void)abcd_updateAcquire(line,
                                     ABCD_YAW_TARGET_CD_DEG100,
                                     ABCD_STATE_DA_LINE);
            break;

        case ABCD_STATE_DA_LINE:
            if (abcd_updateArcLine(line, ABCD_YAW_TARGET_DONE_DEG100) != 0U) {
                g_abcdPointCount = 4U;
                abcd_setYawTarget(ABCD_YAW_TARGET_DONE_DEG100);
                abcd_brake(ABCD_STATE_DONE, "done");
            }
            break;

        case ABCD_STATE_DONE:
            abcd_brake(ABCD_STATE_DONE, "done");
            break;

        case ABCD_STATE_FAIL:
        case ABCD_STATE_OFF:
        default:
            abcd_brake(ABCD_STATE_FAIL, "invalid state");
            break;
    }
}

static void step6_lineFollowUpdate(void)
{
    line_position_t line;

    if (g_driveMode == DRIVE_IMU_STRAIGHT) {
        (void)imuDriveStraight();
        g_lastOdomTicks = step7_getOdomTicks();
        return;
    }

    line = lineSensor_readPosition();

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
        abcd_update(&line);
        return;
    }

    if (g_driveMode == DRIVE_GAP_TO_BLACK) {
        if (step7_gapToBlackUpdate(&line) == 0U) {
            return;
        }

        g_driveMode = DRIVE_BRAKE;
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

    if (g_driveMode != DRIVE_RUN) {
        motorStop();
        linePid_reset();
        g_lastOdomTicks = step7_getOdomTicks();
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
        "Commands: A=ABCD auto  g=Step7 lap  G=line  F=arm white->black  I=IMU init  i=IMU sample  Y=IMU straight  "
        "f=fwd jog  v=rev jog  x=coast  b=brake  "
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
    uiFeedback_clear();
    g_driveMode = DRIVE_STANDBY;
    g_lapActive = 0U;
    g_lapDone   = 0U;
    abcd_clearAutoRun();
    step7_clearGapTest();
    motorStop();
    linePid_reset();
}

static const char *imuInitResultName(mpu6050_straight_init_result_t result)
{
    switch (result) {
        case MPU6050_STRAIGHT_INIT_OK:
            return "OK";
        case MPU6050_STRAIGHT_INIT_CAL_FAILED:
            return "CAL_FAILED";
        case MPU6050_STRAIGHT_INIT_NOT_FOUND:
        default:
            return "NOT_FOUND";
    }
}

static void imuPrintState(void)
{
    const mpu6050_straight_state_t *mpu = mpu6050_straight_state();

    UART0_writeString("[IMU] addr=");
    UART0_writeDec32((int32_t)mpu->addr);
    UART0_writeString(" who=");
    UART0_writeDec32((int32_t)mpu->whoAmI);
    UART0_writeString(" ready=");
    UART0_writeDec32((int32_t)mpu->ready);
    UART0_writeString(" fail=");
    UART0_writeDec32((int32_t)mpu->readFailCount);
    UART0_writeString(" biasZ=");
    UART0_writeDec32(mpu->gyroZBiasRaw);
    UART0_writeString(" rawZ=");
    UART0_writeDec32(mpu->gyroZRaw);
    UART0_writeString(" yawRate100=");
    UART0_writeDec32(mpu->yawRateDps100);
    UART0_writeString(" yaw100=");
    UART0_writeDec32(mpu->yawDeg100);
    UART0_writeString(" corr=");
    UART0_writeDec32((int32_t)mpu->correction);
    UART0_writeString("\r\n");
}

static void imuStopMotionForCheck(void)
{
    uiFeedback_clear();
    abcd_clearAutoRun();
    step7_clearGapTest();
    g_lapActive = 0U;
    g_lapDone   = 0U;
    g_driveMode = DRIVE_STANDBY;
    motorStop();
    linePid_reset();
}

static mpu6050_straight_init_result_t imuRunStillCalibrationCore(void)
{
    mpu6050_straight_init_result_t result;

    UART0_writeString("\r\n[IMU] keep car still; calibrating gyroZ for about 2 seconds...\r\n");
    result = mpu6050_straight_startup_init();
    UART0_writeString("[IMU] init=");
    UART0_writeString(imuInitResultName(result));
    UART0_writeString("\r\n");

    if (result == MPU6050_STRAIGHT_INIT_OK) {
        for (uint8_t i = 0U; i < 5U; i++) {
            delay_ms(LINE_PID_PERIOD_MS);
            (void)mpu6050_straight_update(LINE_PID_PERIOD_MS);
        }
    }

    imuPrintState();
    return result;
}

static void imuRunStartupCheck(void)
{
    imuStopMotionForCheck();
    (void)imuRunStillCalibrationCore();
}

static uint8_t buttonStart_readPressed(void)
{
    return (DL_GPIO_readPins(BUTTON_START_PORT, BUTTON_START_PIN) == 0U) ?
               1U :
               0U;
}

static void buttonStart_init(void)
{
    DL_GPIO_initDigitalInputFeatures(BUTTON_START_IOMUX,
                                     DL_GPIO_INVERSION_DISABLE,
                                     DL_GPIO_RESISTOR_PULL_UP,
                                     DL_GPIO_HYSTERESIS_DISABLE,
                                     DL_GPIO_WAKEUP_DISABLE);

    g_buttonRawPressed = buttonStart_readPressed();
    g_buttonStablePressed = g_buttonRawPressed;
    g_buttonDebounceMs = BUTTON_START_DEBOUNCE_MS;
}

static uint8_t buttonStart_pollPressedEvent(void)
{
    uint8_t rawPressed = buttonStart_readPressed();

    if (rawPressed != g_buttonRawPressed) {
        g_buttonRawPressed = rawPressed;
        g_buttonDebounceMs = 0U;
        return 0U;
    }

    if (g_buttonDebounceMs < BUTTON_START_DEBOUNCE_MS) {
        g_buttonDebounceMs++;
        return 0U;
    }

    if (g_buttonStablePressed == g_buttonRawPressed) {
        return 0U;
    }

    g_buttonStablePressed = g_buttonRawPressed;
    return (g_buttonStablePressed != 0U) ? 1U : 0U;
}

static void buttonStart_stopMotion(void)
{
    abcd_clearAutoRun();
    step7_clearGapTest();
    g_lapActive = 0U;
    g_lapDone = 0U;
    g_driveMode = DRIVE_STANDBY;
    motorStop();
    linePid_reset();
}

static void buttonStart_beginCountdown(void)
{
    uiFeedback_clear();
    buttonStart_stopMotion();
    g_buttonStartState = BUTTON_START_COUNTDOWN;
    g_buttonStartElapsedMs = 0U;
    g_buttonCountdownSecond = 0U;

    uiFeedback_playAutoStart();
    UART0_writeString("\r\n[BTN] B21 accepted; IMU calibration starts in 3 seconds. Keep car still.\r\n");
}

static void buttonStart_cancelCountdown(void)
{
    g_buttonStartState = BUTTON_START_IDLE;
    g_buttonStartElapsedMs = 0U;
    g_buttonCountdownSecond = 0U;
    uiFeedback_clear();
    buttonStart_stopMotion();
    UART0_writeString("\r\n[BTN] countdown cancelled\r\n");
}

static void buttonStart_brakeMovingMode(void)
{
    g_buttonStartState = BUTTON_START_IDLE;
    g_buttonStartElapsedMs = 0U;
    g_buttonCountdownSecond = 0U;
    abcd_clearAutoRun();
    step7_clearGapTest();
    g_lapActive = 0U;
    g_lapDone = 0U;
    g_driveMode = DRIVE_BRAKE;
    motorBrake();
    linePid_reset();
    uiFeedback_playFail();
    UART0_writeString("\r\n[BTN] moving mode interrupted; brake only, no IMU calibration\r\n");
}

static void buttonStart_runCalibrationThenAuto(void)
{
    mpu6050_straight_init_result_t result;

    g_buttonStartState = BUTTON_START_IDLE;
    g_buttonStartElapsedMs = 0U;
    g_buttonCountdownSecond = 0U;
    buttonStart_stopMotion();

    UART0_writeString("\r\n[BTN] countdown complete; IMU calibration then A auto start\r\n");
    result = imuRunStillCalibrationCore();

    if (result == MPU6050_STRAIGHT_INIT_OK) {
        g_buttonStartState = BUTTON_START_AUTO_PENDING;
        g_buttonStartElapsedMs = 0U;
        g_buttonCountdownSecond = 0U;
        uiFeedback_playCalOk();
        UART0_writeString("[BTN] IMU calibration OK; A mode starts after feedback\r\n");
    } else {
        uiFeedback_playFail();
        UART0_writeString("[BTN] IMU calibration failed; A mode not started\r\n");
    }
}

static void buttonStart_cancelPendingAuto(void)
{
    g_buttonStartState = BUTTON_START_IDLE;
    g_buttonStartElapsedMs = 0U;
    g_buttonCountdownSecond = 0U;
    uiFeedback_clear();
    buttonStart_stopMotion();
    UART0_writeString("\r\n[BTN] pending A auto start cancelled\r\n");
}

static void buttonStart_updateCountdown1ms(void)
{
    uint8_t secondsElapsed;

    g_buttonStartElapsedMs++;
    secondsElapsed = (uint8_t)(g_buttonStartElapsedMs / 1000U);
    if ((secondsElapsed > g_buttonCountdownSecond) &&
        (g_buttonStartElapsedMs < BUTTON_START_COUNTDOWN_MS)) {
        uint32_t secondsRemaining =
            (BUTTON_START_COUNTDOWN_MS - g_buttonStartElapsedMs + 999U) / 1000U;
        g_buttonCountdownSecond = secondsElapsed;
        uiFeedback_playAutoStart();
        UART0_writeString("[BTN] countdown ");
        UART0_writeDec32((int32_t)secondsRemaining);
        UART0_writeString("\r\n");
    }

    if (g_buttonStartElapsedMs >= BUTTON_START_COUNTDOWN_MS) {
        buttonStart_runCalibrationThenAuto();
    }
}

static void buttonStart_updateAutoPending1ms(void)
{
    g_buttonStartElapsedMs++;
    if (g_buttonStartElapsedMs >= BUTTON_START_AUTO_DELAY_MS) {
        g_buttonStartState = BUTTON_START_IDLE;
        g_buttonStartElapsedMs = 0U;
        g_buttonCountdownSecond = 0U;
        abcd_startAutoRun();
    }
}

static void buttonStart_update1ms(void)
{
    uint8_t pressedEvent = buttonStart_pollPressedEvent();

    if (pressedEvent != 0U) {
        if (g_buttonStartState == BUTTON_START_COUNTDOWN) {
            buttonStart_cancelCountdown();
            return;
        }

        if (g_buttonStartState == BUTTON_START_AUTO_PENDING) {
            buttonStart_cancelPendingAuto();
            return;
        }

        if ((g_driveMode != DRIVE_STANDBY) && (g_driveMode != DRIVE_BRAKE)) {
            buttonStart_brakeMovingMode();
            return;
        }

        buttonStart_beginCountdown();
        return;
    }

    if (g_buttonStartState == BUTTON_START_COUNTDOWN) {
        buttonStart_updateCountdown1ms();
    } else if (g_buttonStartState == BUTTON_START_AUTO_PENDING) {
        buttonStart_updateAutoPending1ms();
    }
}

static void imuRunSampleCheck(void)
{
    const mpu6050_straight_state_t *mpu = mpu6050_straight_state();

    imuStopMotionForCheck();
    if (mpu->ready == 0U) {
        UART0_writeString("\r\n[IMU] not ready; send uppercase I first\r\n");
        imuPrintState();
        return;
    }

    (void)mpu6050_straight_update(LINE_PID_PERIOD_MS);
    UART0_writeString("\r\n");
    imuPrintState();
}

static void imuStartStraightRun(void)
{
    const mpu6050_straight_state_t *mpu = mpu6050_straight_state();

    uiFeedback_clear();
    abcd_clearAutoRun();
    step7_clearGapTest();
    g_lapActive = 0U;
    g_lapDone   = 0U;
    motorStop();
    linePid_reset();

    if (mpu->ready == 0U) {
        g_driveMode = DRIVE_STANDBY;
        UART0_writeString("\r\n[IMU] not ready; send uppercase I first\r\n");
        imuPrintState();
        return;
    }

    step7_resetOdom();
    step7_gapResetStraightController();
    mpu6050_straight_reset();
    g_driveMode = DRIVE_IMU_STRAIGHT;
    UART0_writeString("\r\n[IMU] straight test armed; send x to stop\r\n");
    imuPrintState();
}

static void step6_handleCommand(uint8_t cmd)
{
    if ((cmd < 0x20U) || (cmd > 0x7EU)) {
        return;
    }

    switch (cmd) {
        case '\r':
        case '\n':
            break;
        case 'A':
        case 'a':
            abcd_startAutoRun();
            break;
        case 'g':
            step7_startLapRun();
            break;
        case 'G':
            uiFeedback_clear();
            abcd_clearAutoRun();
            step7_clearGapTest();
            g_lapActive = 0U;
            g_lapDone   = 0U;
            g_driveMode = DRIVE_RUN;
            linePid_reset();
            UART0_writeString("\r\n[RUN] continuous line PID armed\r\n");
            break;
        case 'f':
            uiFeedback_clear();
            abcd_clearAutoRun();
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
        case 'I':
            imuRunStartupCheck();
            break;
        case 'i':
            imuRunSampleCheck();
            break;
        case 'Y':
        case 'y':
            imuStartStraightRun();
            break;
        case 'v':
        case 'V':
            uiFeedback_clear();
            abcd_clearAutoRun();
            step7_clearGapTest();
            g_lapActive = 0U;
            g_lapDone   = 0U;
            g_driveMode = DRIVE_JOG_REV;
            linePid_reset();
            UART0_writeString("\r\n[REV] jog reverse; send x to stop\r\n");
            break;
        case 'x':
        case 'X':
            uiFeedback_clear();
            abcd_clearAutoRun();
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
            uiFeedback_clear();
            abcd_clearAutoRun();
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
            uiFeedback_clear();
            step7_resetOdom();
            abcd_clearAutoRun();
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

static void abcd_printCompactStatus(int32_t encLeft, int32_t encRight)
{
    UART0_writeString("A s=");
    UART0_writeString(abcdStateName(g_abcdState));
    UART0_writeString(" od=");
    UART0_writeDec32(g_abcdStateOdomTicks);
    UART0_writeString(" ln=");
    UART0_writeBinary8(g_lastLineRaw);
    UART0_writeString(" st=");
    UART0_writeString(lineStatusName(g_lastLineStatus));
    UART0_writeString(" n=");
    UART0_writeDec32(g_lastLineActive);
    UART0_writeString(" e=");
    UART0_writeDec32(g_lastLineError);
    UART0_writeString(" c=");
    UART0_writeDec32(g_lastCorrection);
    UART0_writeString(" ec=");
    UART0_writeDec32(g_lastStraightCorrection);
    UART0_writeString(" ic=");
    UART0_writeDec32(g_lastImuCorrection);
    UART0_writeString(" L=");
    UART0_writeDec32(g_lastLeftCmd);
    UART0_writeString(" R=");
    UART0_writeDec32(g_lastRightCmd);
    UART0_writeString(" enc=");
    UART0_writeDec32(encLeft);
    UART0_writeByte('/');
    UART0_writeDec32(encRight);
    UART0_writeString(" y=");
    UART0_writeDec32(g_abcdYawDeg100);
    UART0_writeString(" h=");
    UART0_writeDec32(g_abcdHoldYawDeg100);
    UART0_writeString(" t=");
    UART0_writeDec32(g_abcdYawTargetDeg100);
    UART0_writeString(" lost=");
    UART0_writeDec32(g_abcdLostTicks);
    UART0_writeString(" p=");
    UART0_writeDec32(g_abcdPointCount);
    UART0_writeString("\r\n");
}

static void step6_printStatus(void)
{
    int32_t encLeft;
    int32_t encRight;
    const mpu6050_straight_state_t *mpu = mpu6050_straight_state();

    step6_readLogicalEncoders(&encLeft, &encRight);
    g_lastOdomTicks = step7_getOdomTicks();

    if (g_driveMode == DRIVE_AUTO) {
        abcd_printCompactStatus(encLeft, encRight);
        return;
    }

    UART0_writeString("mode=");
    UART0_writeString(driveModeName(g_driveMode));
    UART0_writeString(" modeId=");
    UART0_writeDec32((int32_t)g_driveMode);
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
    UART0_writeString(" encCorr=");
    UART0_writeDec32(g_lastStraightCorrection);
    UART0_writeString(" imuCorr=");
    UART0_writeDec32(g_lastImuCorrection);
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
    UART0_writeString(" imuReady=");
    UART0_writeDec32((int32_t)mpu->ready);
    UART0_writeString(" imuRate=");
    UART0_writeDec32(mpu->yawRateDps100);
    UART0_writeString(" imuYaw=");
    UART0_writeDec32(mpu->yawDeg100);
    UART0_writeString(" auto=");
    UART0_writeString(abcdStateName(g_abcdState));
    UART0_writeString(" autoOdom=");
    UART0_writeDec32(g_abcdStateOdomTicks);
    UART0_writeString(" autoYaw=");
    UART0_writeDec32(g_abcdYawDeg100);
    UART0_writeString(" autoTgt=");
    UART0_writeDec32(g_abcdYawTargetDeg100);
    UART0_writeString(" autoErr=");
    UART0_writeDec32(g_abcdYawErrorDeg100);
    UART0_writeString(" autoHold=");
    UART0_writeDec32(g_abcdHoldYawDeg100);
    UART0_writeString(" autoBlk=");
    UART0_writeDec32(g_abcdBlackTicks);
    UART0_writeString(" autoLost=");
    UART0_writeDec32(g_abcdLostTicks);
    UART0_writeString(" autoCenter=");
    UART0_writeDec32(g_abcdCenterTicks);
    UART0_writeString(" autoYawOk=");
    UART0_writeDec32(g_abcdYawTicks);
    UART0_writeString(" autoWhite=");
    UART0_writeDec32(g_abcdWhiteReady);
    UART0_writeString(" point=");
    UART0_writeDec32(g_abcdPointCount);
    UART0_writeString("\r\n");
}

/* ---- main ---- */
int main(void)
{
    SYSCFG_DL_init();
    motorStop();
    uiFeedback_init();
    buttonStart_init();
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
    UART0_writeString("UI: PA2 active-low buzzer, PA14 external LED\r\n");
    UART0_writeString("Button: B21/PB21 active-low starts countdown + IMU calibration + A auto\r\n");
    UART0_printHelp();
    step6_printParams();
    step6_printOrientation();
    UART0_writeString("Boot mode is STBY. Send 'A' for ABCD or 'F'/'G' for checks.\r\n\r\n");
    uiFeedback_playBoot();

    while (1) {
        if (g_uartCommand != 0U) {
            uint8_t cmd = g_uartCommand;
            g_uartCommand = 0U;
            step6_handleCommand(cmd);
        }

        buttonStart_update1ms();

        pidElapsedMs += 1U;
        if (pidElapsedMs >= LINE_PID_PERIOD_MS) {
            pidElapsedMs = 0U;
            step6_lineFollowUpdate();
        }

        statusElapsedMs += 1U;
        uint32_t statusPeriodMs =
            (g_driveMode == DRIVE_AUTO) ? AUTO_STATUS_PRINT_PERIOD_MS
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

        uiFeedback_update1ms();
        delay_ms(1U);
    }
}

/* ---- UART RX echo ---- */
void UART_0_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_0_INST)) {
        case DL_UART_MAIN_IIDX_RX: {
            uint8_t data = (uint8_t)DL_UART_Main_readData(UART_0_INST);
            if ((data == '\r') || (data == '\n')) {
                UART0_writeByte(data);
                break;
            }
            if ((data < 0x20U) || (data > 0x7EU)) {
                break;
            }
            g_uartCommand = data;
            UART0_writeByte(data);
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
