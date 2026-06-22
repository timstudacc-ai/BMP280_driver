/**
 * @file bmp280_i2c.c
 * @brief I2C porting layer implementation for BMP280 sensor
 */
#include "bmp280_i2c.h" 
#include "i2c.h"
#include <stdint.h>

/**
 * @brief  Initializes the I2C interface for BMP280 and verifies communication
 * @param  bmp_device Pointer to the BMP280 interface structure
 * @param  device_address Pointer to the I2C device address (uint8_t)
 * @retval BMP280_StatusTypeDef status of the initialization
 */
BMP280_StatusTypeDef BMP280_I2C_Init(BMP280_Interface *bmp_device, void *device_address){

    /* Check for null pointers to avoid hard faults */
    if(bmp_device == NULL || device_address == NULL){
        return BMP280_ERR_I2C;
    }
    
    /* Assign I2C specific function pointers to the interface */
    bmp_device->bus_read = BMP280_I2C_Read;
    bmp_device->bus_write = BMP280_I2C_Write;
    bmp_device->bus_read_IT = BMP280_I2C_Read_IT;
    bmp_device->bus_read_DMA = BMP280_I2C_Read_DMA;
    bmp_device->bus_write_DMA = NULL; /* Not implemented for I2C */
    bmp_device->intf_ptr = device_address;
    
    uint8_t check = 0;
    
    /* Verify communication by reading the device ID register */
    if (BMP280_I2C_Read((void*)device_address, BMP280_REG_ID, &check, 1) != BMP280_OK)
    {
        return BMP280_ERR_I2C;
    }
    
    /* Call the main driver initialization immediately after setting up I2C */
    return BMP280_Init(bmp_device);
}

/**
 * @brief  Reads data from BMP280 via I2C using blocking mode
 * @param  intf_ptr Pointer to the I2C address
 * @param  reg_addr Register address to read from
 * @param  data Pointer to the data buffer
 * @param  len Number of bytes to read
 * @retval BMP280_StatusTypeDef status of the operation
 */
BMP280_StatusTypeDef BMP280_I2C_Read(void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    /* Cast the generic pointer to uint8_t to get the device address */
    uint8_t dev_addr = (*(uint8_t*)intf_ptr);
    
    /* Perform blocking I2C memory read with a 100ms timeout */
    if(HAL_I2C_Mem_Read(&hi2c2, (dev_addr << 1U), reg_addr, 1, data, len, 100) == HAL_OK)
    {
        return BMP280_OK; /* Success */
    }
    return BMP280_ERR_I2C; /* Error */
}
BMP280_StatusTypeDef BMP280_I2C_Read_DMA(void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    /* Cast the generic pointer to uint8_t to get the device address */
    uint8_t dev_addr = (*(uint8_t*)intf_ptr);
    
    /* Perform DMA-based I2C memory */
    if(HAL_I2C_Mem_Read_DMA(&hi2c2, (dev_addr << 1U), reg_addr, 1, data, len) == HAL_OK)
    {
        return BMP280_OK; /* Success */
    }
    return BMP280_ERR_I2C; /* Error */
}

/**
 * @brief  Writes data to BMP280 via I2C using blocking mode
 * @param  intf_ptr Pointer to the I2C address
 * @param  reg_addr Register address to write to
 * @param  data Pointer to the data buffer
 * @param  len Number of bytes to write
 * @retval BMP280_StatusTypeDef status of the operation
 */
BMP280_StatusTypeDef BMP280_I2C_Write(void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    /* Cast the generic pointer to uint8_t to get the device address */
    uint8_t dev_addr = (*(uint8_t*)intf_ptr);
    
    /* Perform blocking I2C memory write with a 100ms timeout */
    if(HAL_I2C_Mem_Write(&hi2c2, (dev_addr << 1U), reg_addr, 1, data, len, 100) == HAL_OK)
    {
        return BMP280_OK;
    }
    return BMP280_ERR_I2C;
}

/**
 * @brief  Reads data from BMP280 via I2C in non-blocking mode (Interrupt)
 * @param  intf_ptr Pointer to the I2C address
 * @param  reg_addr Register address to read from
 * @param  data Pointer to the data buffer
 * @param  len Number of bytes to read
 * @retval BMP280_StatusTypeDef status of the operation
 */
BMP280_StatusTypeDef BMP280_I2C_Read_IT(void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    /* Cast the generic pointer to uint8_t to get the device address */
    uint8_t dev_addr = (*(uint8_t*)intf_ptr);
    
    /* Initiate a non-blocking I2C memory read using interrupts */
    if(HAL_I2C_Mem_Read_IT(&hi2c2, (dev_addr << 1U), reg_addr, 1, data, len) == HAL_OK)
    {
        return BMP280_OK;
    }
    return BMP280_ERR_I2C;
}


void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance == I2C2)
  {
    BMP280_RX_Cplt_Callback();
  }
}