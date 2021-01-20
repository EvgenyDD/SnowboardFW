#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>

void mpu6050_init(void);
int mpu6050_read_acc_gyro(uint32_t diff_ms);

extern float accel_filt[3], gyro_filt[3];
extern int16_t accel_raw[3], gyro_raw[3];

#endif // MPU6050_H