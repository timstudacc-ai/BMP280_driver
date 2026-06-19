/**
 * @file bmp.c
 * @brief BMP280 Sensor Driver Implementation
 */
#include "bmp.h"
#include <stdint.h>
#include <stddef.h>

/* USER CODE BEGIN Private variables */


/* Volatile variables for IT-based non-blocking reads */
static volatile BMP280_ReadStateTypeDef bmp280_read_state = BMP280_READ_STATE_IDLE;
static volatile uint8_t bmp280_data_buffer[3] = {0}; /* Used for holding 3 bytes of raw temp or press data */


/* Calibration Data */
static BMP280_CalibData bmp280_calib;
static int32_t t_fine = 0; /* Carries fine temperature for pressure compensation */
/* USER CODE END Private variables */

/* USER CODE BEGIN Private functions */

/**
 * @brief  Returns temperature in DegC, resolution is 0.01 DegC.
 *         Output value of "5123" equals 51.23 DegC.
 */
static int32_t bmp280_compensate_T_int32(int32_t adc_T)
{
    int32_t var1, var2, T;

    var1 = ((((adc_T >> 3) - ((int32_t)bmp280_calib.dig_T1 << 1))) * ((int32_t)bmp280_calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)bmp280_calib.dig_T1)) * ((adc_T >> 4) - ((int32_t)bmp280_calib.dig_T1))) >> 12) * ((int32_t)bmp280_calib.dig_T3)) >> 14;

    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

/**
 * @brief  Returns pressure in Pa as unsigned 32 bit integer.
 *         Output value of "96386" equals 96386 Pa = 963.86 hPa.
 */
static uint32_t bmp280_compensate_P_int32(int32_t adc_P)
{
    int32_t var1, var2;
    uint32_t p;

    var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)bmp280_calib.dig_P6);
    var2 = var2 + ((var1 * ((int32_t)bmp280_calib.dig_P5)) << 1);
    var2 = (var2 >> 2) + (((int32_t)bmp280_calib.dig_P4) << 16);

    var1 = (((bmp280_calib.dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((int32_t)bmp280_calib.dig_P2) * var1) >> 1)) >> 18;
    var1 = ((((32768 + var1)) * ((int32_t)bmp280_calib.dig_P1)) >> 15);

    if (var1 == 0)
    {
        return 0; // avoid exception caused by division by zero
    }

    p = (((uint32_t)(((int32_t)1048576) - adc_P) - (var2 >> 12))) * 3125;
    if (p < 0x80000000)
    {
        p = (p << 1) / ((uint32_t)var1);
    }
    else
    {
        p = (p / (uint32_t)var1) * 2;
    }

    var1 = (((int32_t)bmp280_calib.dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
    var2 = (((int32_t)(p >> 2)) * ((int32_t)bmp280_calib.dig_P8)) >> 13;
    p = (uint32_t)((int32_t)p + ((var1 + var2 + bmp280_calib.dig_P7) >> 4));
    return p;
}

/* USER CODE END Private functions */

/* USER CODE BEGIN Exported functions */

BMP280_StatusTypeDef BMP280_Init(Bmp_280_Interface *device)
{
    uint8_t calib_buf[24] = {0}; /* MISRA C Compliance: Always initialize variables */

    if (device == NULL || device->bus_read == NULL || device->bus_write == NULL || device->bus_read_IT == NULL)
    {
        return BMP280_ERR_NULL_PTR;
    }

        /* 3. Read 24 bytes of calibration data from NVM */
        if (device->bus_read(device->intf_ptr, BMP280_REG_CALIB_START, calib_buf, 24) != 0)
        {
            return BMP280_ERR_I2C;
        }

        /* Unpack Little-Endian calibration data */
        bmp280_calib.dig_T1 = (uint16_t)((calib_buf[1] << 8) | calib_buf[0]);
        bmp280_calib.dig_T2 = (int16_t)((calib_buf[3] << 8) | calib_buf[2]);
        bmp280_calib.dig_T3 = (int16_t)((calib_buf[5] << 8) | calib_buf[4]);

        bmp280_calib.dig_P1 = (uint16_t)((calib_buf[7] << 8) | calib_buf[6]);
        bmp280_calib.dig_P2 = (int16_t)((calib_buf[9] << 8) | calib_buf[8]);
        bmp280_calib.dig_P3 = (int16_t)((calib_buf[11] << 8) | calib_buf[10]);
        bmp280_calib.dig_P4 = (int16_t)((calib_buf[13] << 8) | calib_buf[12]);
        bmp280_calib.dig_P5 = (int16_t)((calib_buf[15] << 8) | calib_buf[14]);
        bmp280_calib.dig_P6 = (int16_t)((calib_buf[17] << 8) | calib_buf[16]);
        bmp280_calib.dig_P7 = (int16_t)((calib_buf[19] << 8) | calib_buf[18]);
        bmp280_calib.dig_P8 = (int16_t)((calib_buf[21] << 8) | calib_buf[20]);
        bmp280_calib.dig_P9 = (int16_t)((calib_buf[23] << 8) | calib_buf[22]);

        return BMP280_OK;

        
}

BMP280_StatusTypeDef BMP280_SetMode(Bmp_280_Interface *device, uint8_t mode)
{
    uint8_t data = 0;
    if (device == NULL)
    {
        return BMP280_ERR_NULL_PTR;
    }

    if (device->bus_read(device->intf_ptr, BMP280_REG_CTRL_MEAS, &data, 1) != 0)
    {
        return BMP280_ERR_I2C;
    }

    data &= ~BMP280_CTRL_MEAS_MODE_MSK;
    data |= mode;

    if (device->bus_write(device->intf_ptr, BMP280_REG_CTRL_MEAS, &data, 1) != 0)
    {
        return BMP280_ERR_I2C;
    }
    return BMP280_OK;
}

BMP280_StatusTypeDef BMP280_SetOversampling(Bmp_280_Interface *device, uint8_t osrs_t, uint8_t osrs_p)
{
    uint8_t data = 0;
    if (device == NULL)
    {
        return BMP280_ERR_NULL_PTR;
    }

    if (device->bus_read(device->intf_ptr, BMP280_REG_CTRL_MEAS, &data, 1) != 0)
    {
        return BMP280_ERR_I2C;
    }

    data &= ~(BMP280_CTRL_MEAS_OSRS_T_MSK | BMP280_CTRL_MEAS_OSRS_P_MSK);
    data |= (osrs_t | osrs_p);

    if (device->bus_write(device->intf_ptr, BMP280_REG_CTRL_MEAS, &data, 1) != 0)
    {
        return BMP280_ERR_I2C;
    }
    return BMP280_OK;
}

BMP280_StatusTypeDef BMP280_SetConfig(Bmp_280_Interface *device, uint8_t standby_time, uint8_t filter)
{
    uint8_t ctrl_meas = 0;
    uint8_t config_data = 0;
    uint8_t current_mode = 0;

    if (device == NULL)
    {
        return BMP280_ERR_NULL_PTR;
    }

    /* 1. Read current CTRL_MEAS to save mode */
    if (device->bus_read(device->intf_ptr, BMP280_REG_CTRL_MEAS, &ctrl_meas, 1) != 0)
    {
        return BMP280_ERR_I2C;
    }
    current_mode = ctrl_meas & BMP280_CTRL_MEAS_MODE_MSK;

    /* 2. Put sensor to SLEEP if it's not already */
    if (current_mode != BMP280_MODE_SLEEP)
    {
        uint8_t sleep_mode_cmd = (ctrl_meas & ~BMP280_CTRL_MEAS_MODE_MSK) | BMP280_MODE_SLEEP;
        if (device->bus_write(device->intf_ptr, BMP280_REG_CTRL_MEAS, &sleep_mode_cmd, 1) != 0)
        {
            return BMP280_ERR_I2C;
        }
        return BMP280_OK;
    }


    /* 3. Write to CONFIG register */
    if (device->bus_read(device->intf_ptr, BMP280_REG_CONFIG, &config_data, 1) != 0)
    {
        return BMP280_ERR_I2C;
    }
    config_data &= ~(BMP280_CONFIG_T_SB_MSK | BMP280_CONFIG_FILTER_MSK);
    config_data |= (standby_time | filter);

    if (device->bus_write(device->intf_ptr, BMP280_REG_CONFIG, &config_data, 1) != 0)
    {
        return BMP280_ERR_I2C;
    }

    /* 4. Restore original mode if necessary */
    if (current_mode != BMP280_MODE_SLEEP)
    {
        if (device->bus_write(device->intf_ptr, BMP280_REG_CTRL_MEAS, &ctrl_meas, 1) != 0)
        {
            return BMP280_ERR_I2C;
        }
    }

    return BMP280_OK;
}

BMP280_StatusTypeDef BMP280_ReadTemperature_IT(Bmp_280_Interface *device)
{
    if (device == NULL || device->bus_read_IT == NULL)
    {
        return BMP280_ERR_NULL_PTR;
    }

    if (bmp280_read_state == BMP280_READ_STATE_TEMP_BUSY || bmp280_read_state == BMP280_READ_STATE_PRESS_BUSY)
    {
        return BMP280_ERR_BUSY;
    }

    bmp280_read_state = BMP280_READ_STATE_TEMP_BUSY;

    if (device->bus_read_IT(device->intf_ptr, BMP280_REG_TEMP_MSB, (uint8_t *)bmp280_data_buffer, 3U) != 0)
    {
        bmp280_read_state = BMP280_READ_STATE_ERROR;
        return BMP280_ERR_I2C;
    }

    return BMP280_OK;
}

BMP280_StatusTypeDef BMP280_ReadPressure_IT(Bmp_280_Interface *device)
{
    if (device == NULL || device->bus_read_IT == NULL)
    {
        return BMP280_ERR_NULL_PTR;
    }

    if (bmp280_read_state == BMP280_READ_STATE_TEMP_BUSY || bmp280_read_state == BMP280_READ_STATE_PRESS_BUSY)
    {
        return BMP280_ERR_BUSY;
    }

    if (device->bus_read_IT(device->intf_ptr, BMP280_REG_PRESS_MSB, (uint8_t *)bmp280_data_buffer, 3U) != 0)
    {
        bmp280_read_state = BMP280_READ_STATE_ERROR;
        return BMP280_ERR_I2C;
    }

    return BMP280_OK;
}

BMP280_ReadStateTypeDef BMP280_GetReadState(void)
{
    return bmp280_read_state;
}

int32_t BMP280_Convert_RawTemperature(void)
{
    int32_t raw_val;

    if (bmp280_read_state == BMP280_READ_STATE_TEMP_READY)
    {
        raw_val = ((int32_t)bmp280_data_buffer[0] << 12) |
                  ((int32_t)bmp280_data_buffer[1] << 4) |
                  ((int32_t)bmp280_data_buffer[2] >> 4);

        bmp280_read_state = BMP280_READ_STATE_IDLE;
        return bmp280_compensate_T_int32(raw_val);
    }
    return 0; /* Or keep previous value if we stored it globally */
}

uint32_t BMP280_Convert_RawPressure(void)
{
    int32_t raw_val;

    if (bmp280_read_state == BMP280_READ_STATE_PRESS_READY)
    {
        raw_val = ((int32_t)bmp280_data_buffer[0] << 12) |
                  ((int32_t)bmp280_data_buffer[1] << 4) |
                  ((int32_t)bmp280_data_buffer[2] >> 4);

        bmp280_read_state = BMP280_READ_STATE_IDLE;
        return bmp280_compensate_P_int32(raw_val);
    }
    return 0;
}

void BMP280_Rx_CpltCallback(void)
{
    /* Strictly minimal ISR work: Just set the ready flag! */
    if (bmp280_read_state == BMP280_READ_STATE_TEMP_BUSY)
    {
        bmp280_read_state = BMP280_READ_STATE_TEMP_READY;
    }
    else if (bmp280_read_state == BMP280_READ_STATE_PRESS_BUSY)
    {
        bmp280_read_state = BMP280_READ_STATE_PRESS_READY;
    }
}

/* USER CODE END Exported functions */
