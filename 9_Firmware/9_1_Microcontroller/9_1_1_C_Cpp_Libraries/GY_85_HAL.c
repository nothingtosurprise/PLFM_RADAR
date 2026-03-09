#include "GY_85_HAL.h"

static int16_t g_offx = 0, g_offy = 0, g_offz = 0;

// ---------------- Internal Functions ---------------- //
static void GY85_SetAccelerometer(void);
static void GY85_ReadAccelerometer(GY85_t *imu);
static void GY85_SetCompass(void);
static void GY85_ReadCompass(GY85_t *imu);
static void GY85_SetGyro(void);
static void GY85_GyroCalibrate(void);
static void GY85_ReadGyro(GY85_t *imu);

// ---------------- Initialization ---------------- //
void GY85_Init(void)
{
    GY85_SetAccelerometer();
    GY85_SetCompass();
    GY85_SetGyro();
}

// ---------------- Update All Sensors ---------------- //
void GY85_Update(GY85_t *imu)
{
    GY85_ReadAccelerometer(imu);
    GY85_ReadCompass(imu);
    GY85_ReadGyro(imu);
}

// ---------------- Accelerometer ---------------- //
static void GY85_SetAccelerometer(void)
{
    uint8_t data[2];
    data[0] = 0x31; data[1] = 0x01;
    HAL_I2C_Master_Transmit(&hi2c3, ADXL345_ADDR, data, 2, HAL_MAX_DELAY);

    data[0] = 0x2D; data[1] = 0x08;
    HAL_I2C_Master_Transmit(&hi2c3, ADXL345_ADDR, data, 2, HAL_MAX_DELAY);
}

static void GY85_ReadAccelerometer(GY85_t *imu)
{
    uint8_t reg = 0x32;
    uint8_t buf[6];
    HAL_I2C_Master_Transmit(&hi2c3, ADXL345_ADDR, &reg, 1, HAL_MAX_DELAY);
    HAL_I2C_Master_Receive(&hi2c3, ADXL345_ADDR, buf, 6, HAL_MAX_DELAY);

    imu->ax = (int16_t)((buf[1] << 8) | buf[0]);
    imu->ay = (int16_t)((buf[3] << 8) | buf[2]);
    imu->az = (int16_t)((buf[5] << 8) | buf[4]);
}

// ---------------- Compass ---------------- //
static void GY85_SetCompass(void)
{
    uint8_t data[2] = {0x02, 0x00};
    HAL_I2C_Master_Transmit(&hi2c3, HMC5883_ADDR, data, 2, HAL_MAX_DELAY);
}

static void GY85_ReadCompass(GY85_t *imu)
{
    uint8_t reg = 0x03;
    uint8_t buf[6];
    HAL_I2C_Master_Transmit(&hi2c3, HMC5883_ADDR, &reg, 1, HAL_MAX_DELAY);
    HAL_I2C_Master_Receive(&hi2c3, HMC5883_ADDR, buf, 6, HAL_MAX_DELAY);

    imu->mx = (int16_t)((buf[0] << 8) | buf[1]);
    imu->mz = (int16_t)((buf[2] << 8) | buf[3]);
    imu->my = (int16_t)((buf[4] << 8) | buf[5]);
}

// ---------------- Gyroscope ---------------- //
static void GY85_SetGyro(void)
{
    uint8_t data[2];
    data[0] = 0x3E; data[1] = 0x00;
    HAL_I2C_Master_Transmit(&hi2c3, ITG3200_ADDR, data, 2, HAL_MAX_DELAY);

    data[0] = 0x15; data[1] = 0x07;
    HAL_I2C_Master_Transmit(&hi2c3, ITG3200_ADDR, data, 2, HAL_MAX_DELAY);

    data[0] = 0x16; data[1] = 0x1E;
    HAL_I2C_Master_Transmit(&hi2c3, ITG3200_ADDR, data, 2, HAL_MAX_DELAY);

    data[0] = 0x17; data[1] = 0x00;
    HAL_I2C_Master_Transmit(&hi2c3, ITG3200_ADDR, data, 2, HAL_MAX_DELAY);

    HAL_Delay(10);
    GY85_GyroCalibrate();
}

static void GY85_GyroCalibrate(void)
{
    int32_t tmpx = 0, tmpy = 0, tmpz = 0;
    GY85_t imu;

    for(uint8_t i = 0; i < 10; i++)
    {
        HAL_Delay(10);
        GY85_ReadGyro(&imu);
        tmpx += imu.gx;
        tmpy += imu.gy;
        tmpz += imu.gz;
    }

    g_offx = tmpx / 10;
    g_offy = tmpy / 10;
    g_offz = tmpz / 10;
}

static void GY85_ReadGyro(GY85_t *imu)
{
    uint8_t reg = 0x1B;
    uint8_t buf[8];
    HAL_I2C_Master_Transmit(&hi2c3, ITG3200_ADDR, &reg, 1, HAL_MAX_DELAY);
    HAL_I2C_Master_Receive(&hi2c3, ITG3200_ADDR, buf, 8, HAL_MAX_DELAY);

    imu->gx = ((int16_t)((buf[2] << 8) | buf[3])) - g_offx;
    imu->gy = ((int16_t)((buf[4] << 8) | buf[5])) - g_offy;
    imu->gz = ((int16_t)((buf[6] << 8) | buf[7])) - g_offz;
}
