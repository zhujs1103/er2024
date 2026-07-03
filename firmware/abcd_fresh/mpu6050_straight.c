#include "mpu6050_straight.h"

#include "ti_msp_dl_config.h"

#include <stdbool.h>
#include <stdint.h>

/*
 * Keep these pin definitions outside ti_msp_dl_config.h because SysConfig
 * rewrites generated files before each Keil build.
 */
#ifndef GPIO_MPU_I2C_PORT
#define GPIO_MPU_I2C_PORT      (GPIOB)
#define GPIO_MPU_I2C_SCL_PIN   (DL_GPIO_PIN_2)
#define GPIO_MPU_I2C_SCL_IOMUX (IOMUX_PINCM15)
#define GPIO_MPU_I2C_SDA_PIN   (DL_GPIO_PIN_3)
#define GPIO_MPU_I2C_SDA_IOMUX (IOMUX_PINCM16)
#endif

#define MPU6050_I2C_DELAY_US           (5U)
#define MPU6050_I2C_HIGH_TIMEOUT_US    (1000U)
#define MPU6050_ADDR_LOW               (0x68U)
#define MPU6050_ADDR_HIGH              (0x69U)
#define MPU6050_REG_SMPLRT_DIV         (0x19U)
#define MPU6050_REG_CONFIG             (0x1AU)
#define MPU6050_REG_GYRO_CONFIG        (0x1BU)
#define MPU6050_REG_ACCEL_CONFIG       (0x1CU)
#define MPU6050_REG_ACCEL_XOUT_H       (0x3BU)
#define MPU6050_REG_PWR_MGMT_1         (0x6BU)
#define MPU6050_REG_PWR_MGMT_2         (0x6CU)
#define MPU6050_REG_WHO_AM_I           (0x75U)
#define MPU6050_CALIBRATION_SAMPLES    (100U)
#define MPU6050_CALIBRATION_PERIOD_MS  (20U)
#define MPU6050_MIN_CALIBRATION_OK     (60U)
#define MPU6050_GYRO_LSB_PER_DPS       (131L)
#define MPU6050_STRAIGHT_KZ_NUM        (4L)
#define MPU6050_STRAIGHT_KZ_DEN        (100L)
#define MPU6050_STRAIGHT_KYAW_NUM      (0L)
#define MPU6050_STRAIGHT_KYAW_DEN      (100L)
#define MPU6050_STRAIGHT_CORR_MAX      (40)
#define MPU6050_YAW_LIMIT_DEG100       (3000L)
#define MPU6050_READ_FAIL_LIMIT        (5U)

typedef struct {
    int16_t accelX;
    int16_t accelY;
    int16_t accelZ;
    int16_t tempRaw;
    int16_t gyroX;
    int16_t gyroY;
    int16_t gyroZ;
} mpu6050_sample_t;

static mpu6050_straight_state_t g_mpuState = {
    MPU6050_ADDR_LOW, 0U, 0U, 0U, 0, 0, 0, 0, 0
};

static void delayMs(uint32_t ms)
{
    delay_cycles((CPUCLK_FREQ / 1000U) * ms);
}

static void delayUs(uint32_t us)
{
    delay_cycles((CPUCLK_FREQ / 1000000U) * us);
}

static int16_t clampInt16(int32_t value, int16_t minVal, int16_t maxVal)
{
    if (value < minVal) {
        return minVal;
    }
    if (value > maxVal) {
        return maxVal;
    }
    return (int16_t)value;
}

static int32_t clampInt32(int32_t value, int32_t minVal, int32_t maxVal)
{
    if (value < minVal) {
        return minVal;
    }
    if (value > maxVal) {
        return maxVal;
    }
    return value;
}

static void mpuI2cLineInputPullup(uint32_t pincmIndex)
{
    DL_GPIO_initDigitalInputFeatures(pincmIndex, DL_GPIO_INVERSION_DISABLE,
                                     DL_GPIO_RESISTOR_PULL_UP,
                                     DL_GPIO_HYSTERESIS_DISABLE,
                                     DL_GPIO_WAKEUP_DISABLE);
}

static void mpuI2cDriveLow(GPIO_Regs *port, uint32_t pin)
{
    DL_GPIO_clearPins(port, pin);
    DL_GPIO_enableOutput(port, pin);
}

static void mpuI2cRelease(GPIO_Regs *port, uint32_t pin)
{
    DL_GPIO_disableOutput(port, pin);
}

static bool mpuI2cReadPin(GPIO_Regs *port, uint32_t pin)
{
    return (DL_GPIO_readPins(port, pin) != 0U);
}

static void mpuI2cSclLow(void)
{
    mpuI2cDriveLow(GPIO_MPU_I2C_PORT, GPIO_MPU_I2C_SCL_PIN);
}

static void mpuI2cSclRelease(void)
{
    mpuI2cRelease(GPIO_MPU_I2C_PORT, GPIO_MPU_I2C_SCL_PIN);
}

static void mpuI2cSdaLow(void)
{
    mpuI2cDriveLow(GPIO_MPU_I2C_PORT, GPIO_MPU_I2C_SDA_PIN);
}

static void mpuI2cSdaRelease(void)
{
    mpuI2cRelease(GPIO_MPU_I2C_PORT, GPIO_MPU_I2C_SDA_PIN);
}

static bool mpuI2cSclIsHigh(void)
{
    return mpuI2cReadPin(GPIO_MPU_I2C_PORT, GPIO_MPU_I2C_SCL_PIN);
}

static bool mpuI2cSdaIsHigh(void)
{
    return mpuI2cReadPin(GPIO_MPU_I2C_PORT, GPIO_MPU_I2C_SDA_PIN);
}

static bool mpuI2cWaitSclHigh(void)
{
    uint32_t waitedUs = 0U;

    while (waitedUs < MPU6050_I2C_HIGH_TIMEOUT_US) {
        if (mpuI2cSclIsHigh()) {
            return true;
        }
        delayUs(MPU6050_I2C_DELAY_US);
        waitedUs += MPU6050_I2C_DELAY_US;
    }

    return false;
}

static void mpuI2cStop(void)
{
    mpuI2cSdaLow();
    delayUs(MPU6050_I2C_DELAY_US);
    mpuI2cSclRelease();
    (void)mpuI2cWaitSclHigh();
    delayUs(MPU6050_I2C_DELAY_US);
    mpuI2cSdaRelease();
    delayUs(MPU6050_I2C_DELAY_US);
}

static bool mpuI2cStart(void)
{
    mpuI2cSdaRelease();
    mpuI2cSclRelease();
    delayUs(MPU6050_I2C_DELAY_US);

    if (!mpuI2cWaitSclHigh() || !mpuI2cSdaIsHigh()) {
        return false;
    }

    mpuI2cSdaLow();
    delayUs(MPU6050_I2C_DELAY_US);
    mpuI2cSclLow();
    delayUs(MPU6050_I2C_DELAY_US);

    return true;
}

static bool mpuI2cWriteByte(uint8_t data)
{
    uint8_t mask;
    bool ack;

    for (mask = 0x80U; mask != 0U; mask >>= 1U) {
        if ((data & mask) != 0U) {
            mpuI2cSdaRelease();
        } else {
            mpuI2cSdaLow();
        }

        delayUs(MPU6050_I2C_DELAY_US);
        mpuI2cSclRelease();
        if (!mpuI2cWaitSclHigh()) {
            return false;
        }
        delayUs(MPU6050_I2C_DELAY_US);
        mpuI2cSclLow();
        delayUs(MPU6050_I2C_DELAY_US);
    }

    mpuI2cSdaRelease();
    delayUs(MPU6050_I2C_DELAY_US);
    mpuI2cSclRelease();
    if (!mpuI2cWaitSclHigh()) {
        return false;
    }
    delayUs(MPU6050_I2C_DELAY_US);
    ack = !mpuI2cSdaIsHigh();
    mpuI2cSclLow();
    delayUs(MPU6050_I2C_DELAY_US);

    return ack;
}

static uint8_t mpuI2cReadByte(bool ack)
{
    uint8_t value = 0U;

    mpuI2cSdaRelease();
    for (uint8_t bit = 0U; bit < 8U; bit++) {
        value <<= 1U;
        delayUs(MPU6050_I2C_DELAY_US);
        mpuI2cSclRelease();
        (void)mpuI2cWaitSclHigh();
        delayUs(MPU6050_I2C_DELAY_US);
        if (mpuI2cSdaIsHigh()) {
            value |= 1U;
        }
        mpuI2cSclLow();
        delayUs(MPU6050_I2C_DELAY_US);
    }

    if (ack) {
        mpuI2cSdaLow();
    } else {
        mpuI2cSdaRelease();
    }

    delayUs(MPU6050_I2C_DELAY_US);
    mpuI2cSclRelease();
    (void)mpuI2cWaitSclHigh();
    delayUs(MPU6050_I2C_DELAY_US);
    mpuI2cSclLow();
    mpuI2cSdaRelease();
    delayUs(MPU6050_I2C_DELAY_US);

    return value;
}

static void mpuI2cRecoverBus(void)
{
    mpuI2cSdaRelease();
    for (uint8_t i = 0U; i < 9U; i++) {
        mpuI2cSclRelease();
        (void)mpuI2cWaitSclHigh();
        delayUs(MPU6050_I2C_DELAY_US);
        mpuI2cSclLow();
        delayUs(MPU6050_I2C_DELAY_US);
    }
    mpuI2cStop();
}

static void mpuI2cInit(void)
{
    mpuI2cLineInputPullup(GPIO_MPU_I2C_SCL_IOMUX);
    mpuI2cLineInputPullup(GPIO_MPU_I2C_SDA_IOMUX);
    DL_GPIO_disableOutput(GPIO_MPU_I2C_PORT,
                          GPIO_MPU_I2C_SCL_PIN | GPIO_MPU_I2C_SDA_PIN);
    mpuI2cRecoverBus();
}

static bool mpuReadRegsFrom(uint8_t addr, uint8_t reg, uint8_t *data,
                            uint8_t len)
{
    bool ok = mpuI2cStart();
    ok = ok && mpuI2cWriteByte((uint8_t)((addr << 1U) | 0U));
    ok = ok && mpuI2cWriteByte(reg);
    ok = ok && mpuI2cStart();
    ok = ok && mpuI2cWriteByte((uint8_t)((addr << 1U) | 1U));

    if (ok) {
        for (uint8_t i = 0U; i < len; i++) {
            data[i] = mpuI2cReadByte(i < (uint8_t)(len - 1U));
        }
    }

    mpuI2cStop();
    return ok;
}

static bool mpuWriteReg(uint8_t reg, uint8_t value)
{
    bool ok = mpuI2cStart();
    ok = ok && mpuI2cWriteByte((uint8_t)((g_mpuState.addr << 1U) | 0U));
    ok = ok && mpuI2cWriteByte(reg);
    ok = ok && mpuI2cWriteByte(value);
    mpuI2cStop();

    return ok;
}

static bool mpuReadRegs(uint8_t reg, uint8_t *data, uint8_t len)
{
    return mpuReadRegsFrom(g_mpuState.addr, reg, data, len);
}

static bool mpuProbe(void)
{
    uint8_t who;

    if (mpuReadRegsFrom(MPU6050_ADDR_LOW, MPU6050_REG_WHO_AM_I, &who, 1U)) {
        g_mpuState.addr   = MPU6050_ADDR_LOW;
        g_mpuState.whoAmI = who;
        return true;
    }

    if (mpuReadRegsFrom(MPU6050_ADDR_HIGH, MPU6050_REG_WHO_AM_I, &who, 1U)) {
        g_mpuState.addr   = MPU6050_ADDR_HIGH;
        g_mpuState.whoAmI = who;
        return true;
    }

    return false;
}

static bool mpuInit(void)
{
    if (!mpuProbe()) {
        return false;
    }

    if (!mpuWriteReg(MPU6050_REG_PWR_MGMT_1, 0x80U)) {
        return false;
    }
    delayMs(100U);

    if (!mpuWriteReg(MPU6050_REG_PWR_MGMT_1, 0x01U)) {
        return false;
    }
    delayMs(10U);

    return mpuWriteReg(MPU6050_REG_PWR_MGMT_2, 0x00U) &&
           mpuWriteReg(MPU6050_REG_SMPLRT_DIV, 0x09U) &&
           mpuWriteReg(MPU6050_REG_CONFIG, 0x03U) &&
           mpuWriteReg(MPU6050_REG_GYRO_CONFIG, 0x00U) &&
           mpuWriteReg(MPU6050_REG_ACCEL_CONFIG, 0x00U);
}

static int16_t readInt16BE(const uint8_t *data)
{
    return (int16_t)((uint16_t)data[0] << 8U | (uint16_t)data[1]);
}

static bool mpuReadSample(mpu6050_sample_t *sample)
{
    uint8_t raw[14];

    if (!mpuReadRegs(MPU6050_REG_ACCEL_XOUT_H, raw, sizeof(raw))) {
        return false;
    }

    sample->accelX  = readInt16BE(&raw[0]);
    sample->accelY  = readInt16BE(&raw[2]);
    sample->accelZ  = readInt16BE(&raw[4]);
    sample->tempRaw = readInt16BE(&raw[6]);
    sample->gyroX   = readInt16BE(&raw[8]);
    sample->gyroY   = readInt16BE(&raw[10]);
    sample->gyroZ   = readInt16BE(&raw[12]);

    return true;
}

static bool mpuCalibrateGyroZ(void)
{
    mpu6050_sample_t sample;
    int32_t sum = 0;
    uint16_t count = 0U;

    for (uint16_t i = 0U; i < MPU6050_CALIBRATION_SAMPLES; i++) {
        if (mpuReadSample(&sample)) {
            sum += sample.gyroZ;
            count++;
        } else {
            mpuI2cRecoverBus();
        }
        delayMs(MPU6050_CALIBRATION_PERIOD_MS);
    }

    if (count < MPU6050_MIN_CALIBRATION_OK) {
        return false;
    }

    g_mpuState.gyroZBiasRaw = sum / (int32_t)count;
    mpu6050_straight_reset();
    return true;
}

mpu6050_straight_init_result_t mpu6050_straight_startup_init(void)
{
    mpuI2cInit();

    if (!mpuInit()) {
        g_mpuState.ready = 0U;
        return MPU6050_STRAIGHT_INIT_NOT_FOUND;
    }

    if (!mpuCalibrateGyroZ()) {
        g_mpuState.ready = 0U;
        return MPU6050_STRAIGHT_INIT_CAL_FAILED;
    }

    g_mpuState.ready = 1U;
    return MPU6050_STRAIGHT_INIT_OK;
}

void mpu6050_straight_reset(void)
{
    g_mpuState.yawDeg100     = 0;
    g_mpuState.yawRateDps100 = 0;
    g_mpuState.correction    = 0;
    g_mpuState.readFailCount = 0U;
}

void mpu6050_straight_force_idle(void)
{
    g_mpuState.correction = 0;
}

int16_t mpu6050_straight_update(uint32_t periodMs)
{
    mpu6050_sample_t sample;
    int32_t rateRaw;
    int32_t correction;

    if (g_mpuState.ready == 0U) {
        g_mpuState.correction = 0;
        return 0;
    }

    if (!mpuReadSample(&sample)) {
        if (g_mpuState.readFailCount < MPU6050_READ_FAIL_LIMIT) {
            g_mpuState.readFailCount++;
        } else {
            g_mpuState.ready = 0U;
        }
        g_mpuState.correction = 0;
        mpuI2cRecoverBus();
        return 0;
    }

    g_mpuState.readFailCount = 0U;
    g_mpuState.gyroZRaw = sample.gyroZ;
    rateRaw = (int32_t)sample.gyroZ - g_mpuState.gyroZBiasRaw;
    g_mpuState.yawRateDps100 = (rateRaw * 100L) / MPU6050_GYRO_LSB_PER_DPS;
    g_mpuState.yawDeg100 +=
        (g_mpuState.yawRateDps100 * (int32_t)periodMs) / 1000L;
    g_mpuState.yawDeg100 = clampInt32(g_mpuState.yawDeg100,
                                       -MPU6050_YAW_LIMIT_DEG100,
                                       MPU6050_YAW_LIMIT_DEG100);

    correction = (g_mpuState.yawRateDps100 * MPU6050_STRAIGHT_KZ_NUM) /
                 MPU6050_STRAIGHT_KZ_DEN;
    correction += (g_mpuState.yawDeg100 * MPU6050_STRAIGHT_KYAW_NUM) /
                  MPU6050_STRAIGHT_KYAW_DEN;
    g_mpuState.correction = clampInt16(correction,
                                       -MPU6050_STRAIGHT_CORR_MAX,
                                       MPU6050_STRAIGHT_CORR_MAX);

    return g_mpuState.correction;
}

const mpu6050_straight_state_t *mpu6050_straight_state(void)
{
    return &g_mpuState;
}
