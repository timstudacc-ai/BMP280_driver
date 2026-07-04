/**
 * @file bmp280.c
 * @brief BMP280 Sensor Driver Implementation
 */
#include "bmp280.h"
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
 * @param  adc_T Raw uncompensated temperature value.
 * @retval int32_t Compensated temperature in 0.01 DegC.
 */
static int32_t bmp280_compensate_T_int32(int32_t adc_T)
{
    int32_t var1, var2, T;

    /* Calculate var1 based on calibration parameters T1 and T2 */
    var1 = ((((adc_T >> 3) - ((int32_t)bmp280_calib.dig_T1 << 1))) * ((int32_t)bmp280_calib.dig_T2)) >> 11;
    
    /* Calculate var2 based on calibration parameters T1 and T3 */
    var2 = (((((adc_T >> 4) - ((int32_t)bmp280_calib.dig_T1)) * ((adc_T >> 4) - ((int32_t)bmp280_calib.dig_T1))) >> 12) * ((int32_t)bmp280_calib.dig_T3)) >> 14;

    /* Store fine temperature for pressure compensation */
    t_fine = var1 + var2;
    
    /* Calculate final temperature */
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

/**
 * @brief  Returns pressure in Pa as unsigned 32 bit integer.
 *         Output value of "96386" equals 96386 Pa = 963.86 hPa.
 * @param  adc_P Raw uncompensated pressure value.
 * @retval uint32_t Compensated pressure in Pascals.
 */
static uint32_t bmp280_compensate_P_int32(int32_t adc_P)
{
    int32_t var1, var2;
    uint32_t p;

    /* Step 1: Calculate var1 and var2 based on t_fine and pressure calibration parameters */
    var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)bmp280_calib.dig_P6);
    var2 = var2 + ((var1 * ((int32_t)bmp280_calib.dig_P5)) << 1);
    var2 = (var2 >> 2) + (((int32_t)bmp280_calib.dig_P4) << 16);

    var1 = (((bmp280_calib.dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((int32_t)bmp280_calib.dig_P2) * var1) >> 1)) >> 18;
    var1 = ((((32768 + var1)) * ((int32_t)bmp280_calib.dig_P1)) >> 15);

    /* Prevent division by zero */
    if (var1 == 0)
    {
        return 0; // avoid exception caused by division by zero
    }

    /* Step 2: Calculate preliminary pressure */
    p = (((uint32_t)(((int32_t)1048576) - adc_P) - (var2 >> 12))) * 3125;
    if (p < 0x80000000)
    {
        p = (p << 1) / ((uint32_t)var1);
    }
    else
    {
        p = (p / (uint32_t)var1) * 2;
    }

    /* Step 3: Apply final compensation using P8, P9 and P7 */
    var1 = (((int32_t)bmp280_calib.dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
    var2 = (((int32_t)(p >> 2)) * ((int32_t)bmp280_calib.dig_P8)) >> 13;
    p = (uint32_t)((int32_t)p + ((var1 + var2 + bmp280_calib.dig_P7) >> 4));
    return p;
}

/* USER CODE END Private functions */

/* USER CODE BEGIN Exported functions */

/**
 * @brief  Initializes the BMP280 sensor and reads calibration data.
 * @param  device Pointer to the BMP280 interface structure.
 * @retval BMP280_StatusTypeDef Status of the initialization.
 */
BMP280_StatusTypeDef BMP280_Init(BMP280_Interface *device)
{
    uint8_t calib_buf[24] = {0}; /* MISRA C Compliance: Always initialize variables */

    /* Verify all required function pointers are provided */
    if (device == NULL || device->bus_read == NULL || device->bus_write == NULL || device->bus_read_IT == NULL || device->bus_read_DMA == NULL)
    {
        return BMP280_ERR_NULL_PTR;
    }

    /* Read 24 bytes of calibration data from NVM */
    if (device->bus_read(device->handle, device->intf_ptr, BMP280_REG_CALIB_START, calib_buf, 24) != 0)
    {
        return BMP280_ERR_COMM;
    }

    /* Unpack Little-Endian calibration data into the calibration structure */
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

/**
 * @brief  Sets the sensor's power mode.
 * @param  device Pointer to the BMP280 interface structure.
 * @param  mode Power mode to set (e.g., BMP280_MODE_NORMAL).
 * @retval BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_Set_Mode(BMP280_Interface *device, uint8_t mode)
{
    uint8_t data = 0;
    if (device == NULL)
    {
        return BMP280_ERR_NULL_PTR;
    }

    /* Read the current measurement control register */
    if (device->bus_read(device->handle, device->intf_ptr, BMP280_REG_CTRL_MEAS, &data, 1) != 0)
    {
        return BMP280_ERR_COMM;
    }

    /* Mask out the old mode and apply the new mode */
    data &= ~BMP280_CTRL_MEAS_MODE_MSK;
    data |= mode;

    /* Write back the updated measurement control register */
    if (device->bus_write(device->handle, device->intf_ptr, BMP280_REG_CTRL_MEAS, &data, 1) != 0)
    {
        return BMP280_ERR_COMM;
    }
    return BMP280_OK;
}

/**
 * @brief  Sets the oversampling rates for temperature and pressure.
 * @param  device Pointer to the BMP280 interface structure.
 * @param  osrs_t Temperature oversampling configuration.
 * @param  osrs_p Pressure oversampling configuration.
 * @retval BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_Set_Oversampling(BMP280_Interface *device, uint8_t osrs_t, uint8_t osrs_p)
{
    uint8_t data = 0;
    if (device == NULL)
    {
        return BMP280_ERR_NULL_PTR;
    }

    /* Read current measurement control register */
    if (device->bus_read(device->handle, device->intf_ptr, BMP280_REG_CTRL_MEAS, &data, 1) != 0)
    {
        return BMP280_ERR_COMM;
    }

    /* Mask out the old oversampling settings and apply new ones */
    data &= ~(BMP280_CTRL_MEAS_OSRS_T_MSK | BMP280_CTRL_MEAS_OSRS_P_MSK);
    data |= (osrs_t | osrs_p);

    /* Write back the updated config register */
    if (device->bus_write(device->handle, device->intf_ptr, BMP280_REG_CTRL_MEAS, &data, 1) != 0)
    {
        return BMP280_ERR_COMM;
    }
    return BMP280_OK;
}

/**
 * @brief  Sets the IIR filter coefficient and standby time.
 *         Enforces SLEEP mode constraint before writing to CONFIG.
 * @param  device Pointer to the BMP280 interface structure.
 * @param  standby_time Standby time configuration (t_sb).
 * @param  filter IIR filter coefficient.
 * @retval BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_Set_Config(BMP280_Interface *device, uint8_t standby_time, uint8_t filter)
{
    uint8_t ctrl_meas = 0;
    uint8_t config_data = 0;
    uint8_t current_mode = 0;

    if (device == NULL)
    {
        return BMP280_ERR_NULL_PTR;
    }

    /* 1. Read current CTRL_MEAS to save mode */
    if (device->bus_read(device->handle, device->intf_ptr, BMP280_REG_CTRL_MEAS, &ctrl_meas, 1) != 0)
    {
        return BMP280_ERR_COMM;
    }
    current_mode = ctrl_meas & BMP280_CTRL_MEAS_MODE_MSK;

    /* 2. Put sensor to SLEEP if it's not already. Configuration requires SLEEP mode. */
    if (current_mode != BMP280_MODE_SLEEP)
    {
        uint8_t sleep_mode_cmd = (ctrl_meas & ~BMP280_CTRL_MEAS_MODE_MSK) | BMP280_MODE_SLEEP;
        if (device->bus_write(device->handle, device->intf_ptr, BMP280_REG_CTRL_MEAS, &sleep_mode_cmd, 1) != 0)
        {
            return BMP280_ERR_COMM;
        }
    }

    /* Read the current config register */
    if (device->bus_read(device->handle, device->intf_ptr, BMP280_REG_CONFIG, &config_data, 1) != 0)
    {
        return BMP280_ERR_COMM;
    }
    
    /* Apply new standby time and filter settings */
    config_data &= ~(BMP280_CONFIG_T_SB_MSK | BMP280_CONFIG_FILTER_MSK);
    config_data |= (standby_time | filter);

    /* Write back to CONFIG register */
    if (device->bus_write(device->handle, device->intf_ptr, BMP280_REG_CONFIG, &config_data, 1) != 0)
    {
        return BMP280_ERR_COMM;
    }

    /* 4. Restore original mode if necessary */
    if (current_mode != BMP280_MODE_SLEEP)
    {
        if (device->bus_write(device->handle, device->intf_ptr, BMP280_REG_CTRL_MEAS, &ctrl_meas, 1) != 0)
        {
            return BMP280_ERR_COMM;
        }
    }

    return BMP280_OK;
}

/**
 * @brief  Starts asynchronous reading of temperature via Interrupts.
 * @param  device Pointer to the BMP280 interface structure.
 * @retval BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_Read_Temperature_IT(BMP280_Interface *device)
{
    if (device == NULL || device->bus_read_IT == NULL)
    {
        return BMP280_ERR_NULL_PTR;
    }

    /* Ensure no other asynchronous operation is ongoing */
    if (bmp280_read_state == BMP280_READ_STATE_TEMP_BUSY || bmp280_read_state == BMP280_READ_STATE_PRESS_BUSY)
    {
        return BMP280_ERR_BUSY;
    }

    /* Mark state as busy for temperature */
    bmp280_read_state = BMP280_READ_STATE_TEMP_BUSY;

    /* Start an interrupt-based read of the 3 temperature registers */
    if (device->bus_read_IT(device->handle, device->intf_ptr, BMP280_REG_TEMP_MSB, (uint8_t *)bmp280_data_buffer, 3U) != 0)
    {
        bmp280_read_state = BMP280_READ_STATE_ERROR;
        return BMP280_ERR_COMM;
    }

    return BMP280_OK;
}

/**
 * @brief  Starts asynchronous reading of pressure via Interrupts.
 * @param  device Pointer to the BMP280 interface structure.
 * @retval BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_Read_Pressure_IT(BMP280_Interface *device)
{
    if (device == NULL || device->bus_read_IT == NULL)
    {
        return BMP280_ERR_NULL_PTR;
    }

    /* Ensure no other asynchronous operation is ongoing */
    if (bmp280_read_state == BMP280_READ_STATE_TEMP_BUSY || bmp280_read_state == BMP280_READ_STATE_PRESS_BUSY)
    {
        return BMP280_ERR_BUSY;
    }
    
    /* Mark state as busy for pressure */
    bmp280_read_state = BMP280_READ_STATE_PRESS_BUSY;

    /* Start an interrupt-based read of the 3 pressure registers */
    if (device->bus_read_IT(device->handle, device->intf_ptr, BMP280_REG_PRESS_MSB, (uint8_t *)bmp280_data_buffer, 3) != 0)
    {
        bmp280_read_state = BMP280_READ_STATE_ERROR;
        return BMP280_ERR_COMM;
    }

    return BMP280_OK;
}

/**
 * @brief  Starts asynchronous reading of pressure via DMA.
 * @param  device Pointer to the BMP280 interface structure.
 * @retval BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_Read_Pressure_DMA(BMP280_Interface *device)
{
    if (device == NULL || device->bus_read_DMA == NULL)
    {
        return BMP280_ERR_NULL_PTR;
    }

    /* Ensure no other asynchronous operation is ongoing */
    if (bmp280_read_state == BMP280_READ_STATE_TEMP_BUSY || bmp280_read_state == BMP280_READ_STATE_PRESS_BUSY)
    {
        return BMP280_ERR_BUSY;
    }
    
    /* Mark state as busy for pressure */
    bmp280_read_state = BMP280_READ_STATE_PRESS_BUSY;

    /* Start a DMA-based read of the 3 pressure registers */
    if (device->bus_read_DMA(device->handle, device->intf_ptr, BMP280_REG_PRESS_MSB, (uint8_t *)bmp280_data_buffer, 3) != 0)
    {
        bmp280_read_state = BMP280_READ_STATE_ERROR;
        return BMP280_ERR_COMM;
    }

    return BMP280_OK;
}

/**
 * @brief  Starts asynchronous reading of temperature via DMA.
 * @param  device Pointer to the BMP280 interface structure.
 * @retval BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_Read_Temperature_DMA(BMP280_Interface *device)
{
    if (device == NULL || device->bus_read_DMA == NULL)
    {
        return BMP280_ERR_NULL_PTR;
    }

    /* Ensure no other asynchronous operation is ongoing */
    if (bmp280_read_state == BMP280_READ_STATE_TEMP_BUSY || bmp280_read_state == BMP280_READ_STATE_PRESS_BUSY)
    {
        return BMP280_ERR_BUSY;
    }
    
    /* Mark state as busy for temperature */
    bmp280_read_state = BMP280_READ_STATE_TEMP_BUSY;

    /* Start a DMA-based read of the 3 temperature registers */
    if (device->bus_read_DMA(device->handle, device->intf_ptr, BMP280_REG_TEMP_MSB, (uint8_t *)bmp280_data_buffer, 3) != 0)
    {
        bmp280_read_state = BMP280_READ_STATE_ERROR;
        return BMP280_ERR_COMM;
    }

    return BMP280_OK;
}


/**
 * @brief  Converts the raw temperature from the internal buffer into 0.01 DegC.
 *         Internal function called by BMP280_Get_Temperature.
 * @retval int32_t Compensated temperature in 0.01 DegC format.
 */
int32_t BMP280_Convert_Raw_Temperature(void)
{
    int32_t raw_val;

    /* Verify that the temperature reading has actually finished */
    if (bmp280_read_state == BMP280_READ_STATE_TEMP_READY)
    {
        /* Reconstruct the 20-bit raw ADC value from the 3 bytes */
        raw_val = ((int32_t)bmp280_data_buffer[0] << 12) |
                  ((int32_t)bmp280_data_buffer[1] << 4) |
                  ((int32_t)bmp280_data_buffer[2] >> 4);

        /* Reset the state machine back to IDLE */
        bmp280_read_state = BMP280_READ_STATE_IDLE;
        
        /* Apply mathematical compensation based on calibration data */
        return bmp280_compensate_T_int32(raw_val);
    }
    return 0; /* Return 0 if data was not ready */
}

/**
 * @brief  Converts the raw pressure from the internal buffer into Pascals.
 *         Internal function called by BMP280_Get_Pressure.
 * @retval uint32_t Compensated pressure in Pascals format.
 */
uint32_t BMP280_Convert_Raw_Pressure(void)
{
    int32_t raw_val;

    /* Verify that the pressure reading has actually finished */
    if (bmp280_read_state == BMP280_READ_STATE_PRESS_READY)
    {
        /* Reconstruct the 20-bit raw ADC value from the 3 bytes */
        raw_val = ((int32_t)bmp280_data_buffer[0] << 12) |
                  ((int32_t)bmp280_data_buffer[1] << 4) |
                  ((int32_t)bmp280_data_buffer[2] >> 4);

        /* Reset the state machine back to IDLE */
        bmp280_read_state = BMP280_READ_STATE_IDLE;
        
        /* Apply mathematical compensation based on calibration data */
        return bmp280_compensate_P_int32(raw_val);
    }
    return 0; /* Return 0 if data was not ready */
}

/**
 * @brief  Non-blocking function to poll and retrieve the temperature.
 *         Initiates a DMA read on the first call and returns BUSY.
 *         Subsequent calls return BUSY until the read completes.
 * @param  device Pointer to the BMP280 interface structure.
 * @param  temperature Pointer to variable where the result (in 0.01 DegC) will be stored.
 * @retval BMP280_StatusTypeDef BMP280_OK if data is ready, BMP280_ERR_BUSY if still reading.
 */
BMP280_StatusTypeDef BMP280_Get_Temperature(BMP280_Interface *device, int32_t *temperature) {
    if (device == NULL || temperature == NULL) {
        return BMP280_ERR_NULL_PTR;
    }

    /* Static state to track internal progression of the non-blocking read */
    static uint8_t temp_read_step = 0;
    BMP280_StatusTypeDef status;

    switch (temp_read_step)
    {
    case 0:
        /* Step 0: Initiate the DMA transaction */
        status = BMP280_Read_Temperature_DMA(device);
        if (status != BMP280_OK) {
            return status;
        }
        
        /* Move to Step 1 and yield the CPU */
        temp_read_step = 1;
        return BMP280_ERR_BUSY;
    
    case 1:
        /* Step 1: Poll the internal state to see if DMA has finished */
        if (bmp280_read_state == BMP280_READ_STATE_TEMP_READY) {
            /* Data is ready, perform mathematical conversion */
            *temperature = BMP280_Convert_Raw_Temperature(); /* Converts and resets state to IDLE */
            
            /* Reset the step counter for the next time this function is called */
            temp_read_step = 0;
            return BMP280_OK;
        }
        else if (bmp280_read_state == BMP280_READ_STATE_ERROR) {
            /* A hardware error occurred during reading */
            temp_read_step = 0;
            return BMP280_ERR_COMM;
        }
        
        /* Still waiting for DMA to complete */
        return BMP280_ERR_BUSY;
    }
    
    /* Fallback catch */
    temp_read_step = 0;
    return BMP280_ERR_BUSY;
}

/**
 * @brief  Non-blocking function to poll and retrieve the pressure.
 *         Initiates a DMA read on the first call and returns BUSY.
 *         Subsequent calls return BUSY until the read completes.
 * @param  device Pointer to the BMP280 interface structure.
 * @param  pressure Pointer to variable where the result (in Pa) will be stored.
 * @retval BMP280_StatusTypeDef BMP280_OK if data is ready, BMP280_ERR_BUSY if still reading.
 */
BMP280_StatusTypeDef BMP280_Get_Pressure(BMP280_Interface *device, uint32_t *pressure) {
    if (device == NULL || pressure == NULL) {
        return BMP280_ERR_NULL_PTR;
    }

    /* Static state to track internal progression of the non-blocking read */
    static uint8_t press_read_step = 0;
    BMP280_StatusTypeDef status;

    switch (press_read_step)
    {
    case 0:
        /* Step 0: Initiate the DMA transaction */
        status = BMP280_Read_Pressure_DMA(device);
        if (status != BMP280_OK) {
            return status;
        }
        
        /* Move to Step 1 and yield the CPU */
        press_read_step = 1;
        return BMP280_ERR_BUSY;
    
    case 1:
        /* Step 1: Poll the internal state to see if DMA has finished */
        if (bmp280_read_state == BMP280_READ_STATE_PRESS_READY) {
            /* Data is ready, perform mathematical conversion */
            *pressure = BMP280_Convert_Raw_Pressure(); /* Converts and resets state to IDLE */
            
            /* Reset the step counter for the next time this function is called */
            press_read_step = 0;
            return BMP280_OK;
        }
        else if (bmp280_read_state == BMP280_READ_STATE_ERROR) {
            /* A hardware error occurred during reading */
            press_read_step = 0;
            return BMP280_ERR_COMM;
        }
        
        /* Still waiting for DMA to complete */
        return BMP280_ERR_BUSY;
    }
    
    /* Fallback catch */
    press_read_step = 0;
    return BMP280_ERR_BUSY;
}

/**
 * @brief  RX Complete Callback to update the internal state machine.
 *         Should be called from the SPI/I2C RX Complete interrupt handler.
 */
void BMP280_RX_Cplt_Callback(void)
{
    /* Strictly minimal ISR work: Just set the ready flag based on the current BUSY state! */
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
