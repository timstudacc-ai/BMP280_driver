/**
 * @file bmp.h
 * @brief BMP280 Sensor Driver Header
 */
#ifndef BMP280_H
#define BMP280_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* USER CODE BEGIN Includes */
#include "main.h" /* For I2C_HandleTypeDef and HAL_StatusTypeDef */
/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */

/* BMP280 Register Memory Map */
#define BMP280_DEVICE_ADRESS (0x76U)   /* device adress */
#define BMP280_REG_CALIB_START (0x88U) /* Start of calibration data */
#define BMP280_REG_CALIB_END (0xA1U)   /* End of calibration data */
#define BMP280_REG_ID (0xD0U)          /* Chip ID register */
#define BMP280_REG_RESET (0xE0U)       /* Soft reset register */
#define BMP280_REG_STATUS (0xF3U)      /* Status register */
#define BMP280_REG_CTRL_MEAS (0xF4U)   /* Control measurement register */
#define BMP280_REG_CONFIG (0xF5U)      /* Configuration register */
#define BMP280_REG_PRESS_MSB (0xF7U)   /* Pressure MSB register */
#define BMP280_REG_PRESS_LSB (0xF8U)   /* Pressure LSB register */
#define BMP280_REG_PRESS_XLSB (0xF9U)  /* Pressure XLSB register */
#define BMP280_REG_TEMP_MSB (0xFAU)    /* Temperature MSB register */
#define BMP280_REG_TEMP_LSB (0xFBU)    /* Temperature LSB register */
#define BMP280_REG_TEMP_XLSB (0xFCU)   /* Temperature XLSB register */

/* Magic Numbers */
#define BMP280_CHIP_ID_MAGIC (0x58U)    /* Expected ID value */
#define BMP280_SOFT_RESET_MAGIC (0xB6U) /* Triggers power-on-reset */

/* Status Register (0xF3) Bit Definitions */
#define BMP280_STATUS_MEASURING_POS (3U)    /* Position of measuring bit */
#define BMP280_STATUS_MEASURING_MSK (0x08U) /* Measuring bit mask */
#define BMP280_STATUS_IM_UPDATE_POS (0U)    /* Position of im_update bit */
#define BMP280_STATUS_IM_UPDATE_MSK (0x01U) /* im_update bit mask */

/* Status Register (0xF3) Values */
#define BMP280_STATUS_MEASURING_ON (0x01U << 3U) /* Measuring in progress */
#define BMP280_STATUS_IM_UPDATE_ON (0x01U << 0U) /* NVM update in progress */

/* Control Measurement Register (0xF4) Bit Definitions */
#define BMP280_CTRL_MEAS_OSRS_T_POS (5U)    /* Position of osrs_t bits */
#define BMP280_CTRL_MEAS_OSRS_T_MSK (0xE0U) /* Mask for osrs_t */
#define BMP280_CTRL_MEAS_OSRS_P_POS (2U)    /* Position of osrs_p bits */
#define BMP280_CTRL_MEAS_OSRS_P_MSK (0x1CU) /* Mask for osrs_p */
#define BMP280_CTRL_MEAS_MODE_POS (0U)      /* Position of mode bits */
#define BMP280_CTRL_MEAS_MODE_MSK (0x03U)   /* Mask for mode */

/* Temperature Oversampling Values (osrs_t) - Shifted to bits 7:5 */
#define BMP280_OSRS_T_SKIP (0x00U << 5U) /* 000: Skipped */
#define BMP280_OSRS_T_1X (0x01U << 5U)   /* 001: oversampling x 1 */
#define BMP280_OSRS_T_2X (0x02U << 5U)   /* 010: oversampling x 2 */
#define BMP280_OSRS_T_4X (0x03U << 5U)   /* 011: oversampling x 4 */
#define BMP280_OSRS_T_8X (0x04U << 5U)   /* 100: oversampling x 8 */
#define BMP280_OSRS_T_16X (0x07U << 5U)  /* 111: oversampling x 16 */

/* Pressure Oversampling Values (osrs_p) - Shifted to bits 4:2 */
#define BMP280_OSRS_P_SKIP (0x00U << 2U) /* 000: Skipped */
#define BMP280_OSRS_P_1X (0x01U << 2U)   /* 001: oversampling x 1 */
#define BMP280_OSRS_P_2X (0x02U << 2U)   /* 010: oversampling x 2 */
#define BMP280_OSRS_P_4X (0x03U << 2U)   /* 011: oversampling x 4 */
#define BMP280_OSRS_P_8X (0x04U << 2U)   /* 100: oversampling x 8 */
#define BMP280_OSRS_P_16X (0x07U << 2U)  /* 111: oversampling x 16 */

/* Modes definitions for mode[1:0] in ctrl_meas - Shifted to bits 1:0 */
#define BMP280_MODE_SLEEP (0x00U)  /* 00: Sleep mode */
#define BMP280_MODE_FORCED (0x01U) /* 01: Forced mode */
#define BMP280_MODE_NORMAL (0x03U) /* 11: Normal mode */

/* Config Register (0xF5) Bit Definitions */
#define BMP280_CONFIG_T_SB_POS (5U)        /* Position of t_sb bits */
#define BMP280_CONFIG_T_SB_MSK (0xE0U)     /* Mask for t_sb */
#define BMP280_CONFIG_FILTER_POS (2U)      /* Position of filter bits */
#define BMP280_CONFIG_FILTER_MSK (0x1CU)   /* Mask for filter */
#define BMP280_CONFIG_SPI3W_EN_POS (0U)    /* Position of spi3w_en bit */
#define BMP280_CONFIG_SPI3W_EN_MSK (0x01U) /* Mask for spi3w_en */

/* Standby Time Values (t_sb) - Shifted to bits 7:5 */
#define BMP280_STANDBY_0_5_MS (0x00U << 5U)  /* 000: 0.5 ms */
#define BMP280_STANDBY_62_5_MS (0x01U << 5U) /* 001: 62.5 ms */
#define BMP280_STANDBY_125_MS (0x02U << 5U)  /* 010: 125 ms */
#define BMP280_STANDBY_250_MS (0x03U << 5U)  /* 011: 250 ms */
#define BMP280_STANDBY_500_MS (0x04U << 5U)  /* 100: 500 ms */
#define BMP280_STANDBY_1000_MS (0x05U << 5U) /* 101: 1000 ms */
#define BMP280_STANDBY_2000_MS (0x06U << 5U) /* 110: 2000 ms */
#define BMP280_STANDBY_4000_MS (0x07U << 5U) /* 111: 4000 ms */

/* IIR Filter Coefficient Values (filter) - Shifted to bits 4:2 */
#define BMP280_FILTER_OFF (0x00U << 2U)      /* 000: Filter off */
#define BMP280_FILTER_COEFF_2 (0x01U << 2U)  /* 001: Coeff = 2 */
#define BMP280_FILTER_COEFF_4 (0x02U << 2U)  /* 010: Coeff = 4 */
#define BMP280_FILTER_COEFF_8 (0x03U << 2U)  /* 011: Coeff = 8 */
#define BMP280_FILTER_COEFF_16 (0x04U << 2U) /* 100: Coeff = 16 */

/* SPI 3-wire Enable Value - Shifted to bit 0 */
#define BMP280_SPI3W_EN_ON (0x01U << 0U) /* 1: 3-wire SPI on */

/* Pressure XLSB Register (0xF9) Bit Definitions */
#define BMP280_PRESS_XLSB_POS (4U)    /* Position of xlsb_p bits */
#define BMP280_PRESS_XLSB_MSK (0xF0U) /* Mask for xlsb_p bits */

/* Temperature XLSB Register (0xFC) Bit Definitions */
#define BMP280_TEMP_XLSB_POS (4U)    /* Position of xlsb_t bits */
#define BMP280_TEMP_XLSB_MSK (0xF0U) /* Mask for xlsb_t bits */

    /* USER CODE END Private defines */

    /* USER CODE BEGIN Prototypes */

    /**
     * @brief Status enumeration for BMP280 operations
     */
    typedef enum
    {
        BMP280_OK = 0x00U,
        BMP280_ERR_I2C = 0x01U,
        BMP280_ERR_ID_MISMATCH = 0x02U,
        BMP280_ERR_NULL_PTR = 0x03U,
        BMP280_ERR_BUSY = 0x04U,
        BMP280_ERR_SPI = 0x05U,
    } BMP280_StatusTypeDef;

    /**
     * @brief State machine enumeration for non-blocking reads
     */
    typedef enum
    {
        BMP280_READ_STATE_IDLE = 0x00U,
        BMP280_READ_STATE_TEMP_BUSY,
        BMP280_READ_STATE_PRESS_BUSY,
        BMP280_READ_STATE_TEMP_READY,
        BMP280_READ_STATE_PRESS_READY,
        BMP280_READ_STATE_ERROR
    } BMP280_ReadStateTypeDef;

    /**
     * @brief Calibration data structure for BMP280 compensation
     */
    typedef struct
    {
        uint16_t dig_T1;
        int16_t dig_T2;
        int16_t dig_T3;
        uint16_t dig_P1;
        int16_t dig_P2;
        int16_t dig_P3;
        int16_t dig_P4;
        int16_t dig_P5;
        int16_t dig_P6;
        int16_t dig_P7;
        int16_t dig_P8;
        int16_t dig_P9;
    } BMP280_CalibData;
    /**
     * @brief Function pointer for synchronous reading
     */
    typedef BMP280_StatusTypeDef (*BMP280_Read_func) (void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len);
    
    /**
     * @brief Function pointer for synchronous writing
     */
    typedef BMP280_StatusTypeDef (*BMP280_Write_func) (void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len);


     typedef BMP280_StatusTypeDef (*BMP280_Read_DMA_func) (void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len);

     typedef BMP280_StatusTypeDef (*BMP280_Write_DMA_func) (void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len);
    
    /**
     * @brief Hardware abstraction interface structure
     */
    typedef struct {
        BMP280_Read_func bus_read;
        BMP280_Write_func bus_write;
        BMP280_Read_func bus_read_IT;
        BMP280_Read_DMA_func bus_read_DMA;
        BMP280_Write_DMA_func bus_write_DMA;
        void *intf_ptr; 
    } Bmp_280_Interface;


    /**
     * @brief  Initializes the BMP280 sensor and reads calibration data.
     * @param  device Pointer to the BMP280 interface structure.
     * @retval BMP280_StatusTypeDef Status of the initialization.
     */
    BMP280_StatusTypeDef BMP280_Init(Bmp_280_Interface *device);

    /**
     * @brief  Sets the sensor's power mode (Sleep, Forced, or Normal).
     * @param  device Pointer to the BMP280 interface structure.
     * @param  mode Desired mode (e.g., BMP280_MODE_NORMAL).
     * @retval BMP280_StatusTypeDef Status of the operation.
     */
    BMP280_StatusTypeDef BMP280_SetMode(Bmp_280_Interface *device, uint8_t mode);

    /**
     * @brief  Sets the oversampling rates for temperature and pressure.
     * @param  device Pointer to the BMP280 interface structure.
     * @param  osrs_t Temperature oversampling configuration.
     * @param  osrs_p Pressure oversampling configuration.
     * @retval BMP280_StatusTypeDef Status of the operation.
     */
    BMP280_StatusTypeDef BMP280_SetOversampling(Bmp_280_Interface *device, uint8_t osrs_t, uint8_t osrs_p);

    /**
     * @brief  Sets the IIR filter coefficient and standby time.
     *         Enforces SLEEP mode constraint before writing to CONFIG.
     * @param  device Pointer to the BMP280 interface structure.
     * @param  standby_time Standby time configuration (t_sb).
     * @param  filter IIR filter coefficient.
     * @retval BMP280_StatusTypeDef Status of the operation.
     */
    BMP280_StatusTypeDef BMP280_SetConfig(Bmp_280_Interface *device, uint8_t standby_time, uint8_t filter);

    /**
     * @brief  Starts asynchronous reading of temperature via Interrupts.
     * @param  device Pointer to the BMP280 interface structure.
     * @retval BMP280_StatusTypeDef Status of the operation.
     */
    BMP280_StatusTypeDef BMP280_ReadTemperature_IT(Bmp_280_Interface *device);

    /**
     * @brief  Starts asynchronous reading of pressure via Interrupts.
     * @param  device Pointer to the BMP280 interface structure.
     * @retval BMP280_StatusTypeDef Status of the operation.
     */
    BMP280_StatusTypeDef BMP280_ReadPressure_IT(Bmp_280_Interface *device);

    /**
     * @brief  Starts asynchronous reading of pressure via DMA.
     * @param  device Pointer to the BMP280 interface structure.
     * @retval BMP280_StatusTypeDef Status of the operation.
     */
    BMP280_StatusTypeDef BMP280_ReadPressure_DMA(Bmp_280_Interface *device);

    /**
     * @brief  Starts asynchronous reading of temperature via DMA.
     * @param  device Pointer to the BMP280 interface structure.
     * @retval BMP280_StatusTypeDef Status of the operation.
     */
    BMP280_StatusTypeDef BMP280_ReadTemperature_DMA(Bmp_280_Interface *device);

    /**
     * @brief  Non-blocking function to poll and retrieve the temperature.
     * @param  device Pointer to the BMP280 interface structure.
     * @param  temperature Pointer to variable where the result (in 0.01 DegC) will be stored.
     * @retval BMP280_StatusTypeDef BMP280_OK if data is ready, BMP280_ERR_BUSY if still reading.
     */
    BMP280_StatusTypeDef BMP280_Get_Temperature(Bmp_280_Interface *device, int32_t *temperature);

    /**
     * @brief  Non-blocking function to poll and retrieve the pressure.
     * @param  device Pointer to the BMP280 interface structure.
     * @param  pressure Pointer to variable where the result (in Pa) will be stored.
     * @retval BMP280_StatusTypeDef BMP280_OK if data is ready, BMP280_ERR_BUSY if still reading.
     */
    BMP280_StatusTypeDef BMP280_Get_Pressure(Bmp_280_Interface *device, uint32_t *pressure);

    /**
     * @brief  Converts the raw temperature from the internal buffer into 0.01 DegC.
     *         Internal function called by BMP280_Get_Temperature.
     * @retval int32_t Compensated temperature in 0.01 DegC format.
     */
    int32_t BMP280_Convert_RawTemperature(void);

    /**
     * @brief  Converts the raw pressure from the internal buffer into Pascals.
     *         Internal function called by BMP280_Get_Pressure.
     * @retval uint32_t Compensated pressure in Pascals format.
     */
    uint32_t BMP280_Convert_RawPressure(void);

    /**
     * @brief  RX Complete Callback to update the internal state machine.
     *         Should be called from the SPI/I2C RX Complete interrupt handler.
     */
    void BMP280_Rx_CpltCallback(void);

    /* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* BMP280_H */
