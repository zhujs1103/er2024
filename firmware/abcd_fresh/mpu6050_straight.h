#ifndef MPU6050_STRAIGHT_H
#define MPU6050_STRAIGHT_H

#include <stdint.h>

/*
 * MPU6050 gyroZ straight-line correction module.
 *
 * Porting requirements:
 *   - Include this .h in the control file and add mpu6050_straight.c to build.
 *   - ti_msp_dl_config.h must provide:
 *       GPIO_MPU_I2C_PORT
 *       GPIO_MPU_I2C_SCL_PIN / GPIO_MPU_I2C_SDA_PIN
 *       GPIO_MPU_I2C_SCL_IOMUX / GPIO_MPU_I2C_SDA_IOMUX
 *       CPUCLK_FREQ
 *   - Call startup_init() once after SYSCFG_DL_init() while the car is still.
 *   - Call update(periodMs) only in the straight segment that needs IMU help.
 */

typedef enum {
    MPU6050_STRAIGHT_INIT_OK = 0,
    MPU6050_STRAIGHT_INIT_NOT_FOUND,
    MPU6050_STRAIGHT_INIT_CAL_FAILED
} mpu6050_straight_init_result_t;

typedef struct {
    uint8_t addr;
    uint8_t whoAmI;
    uint8_t ready;
    uint8_t readFailCount;
    int32_t gyroZBiasRaw;
    int32_t gyroZRaw;
    int32_t yawRateDps100;
    int32_t yawDeg100;
    int16_t correction;
} mpu6050_straight_state_t;

mpu6050_straight_init_result_t mpu6050_straight_startup_init(void);
void mpu6050_straight_reset(void);
void mpu6050_straight_force_idle(void);
int16_t mpu6050_straight_update(uint32_t periodMs);
const mpu6050_straight_state_t *mpu6050_straight_state(void);

#endif /* MPU6050_STRAIGHT_H */
