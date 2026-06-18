/**
 * @file bmp_I2c.h
 * @brief I2C porting layer for BMP280 sensor
 */
#ifndef _BMP_I2C_H
#define _BMP_I2C_H

#include "bmp.h"

/**
 * @brief  Initializes the I2C interface for BMP280
 * @param  bmp_device Pointer to the BMP280 interface structure
 * @param  device_address Pointer to the I2C device address
 * @retval BMP280_StatusTypeDef status of the initialization
 */
BMP280_StatusTypeDef BMP_I2C_Init(Bmp_280_Interface *bmp_device, void *device_address);

/**
 * @brief  Reads data from BMP280 via I2C
 * @param  intf_ptr Pointer to the interface specific data (I2C address)
 * @param  reg_addr Register address to read from
 * @param  data Pointer to the data buffer
 * @param  len Number of bytes to read
 * @retval BMP280_StatusTypeDef status of the operation
 */
BMP280_StatusTypeDef BMP280_I2C_Read(void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len);

/**
 * @brief  Writes data to BMP280 via I2C
 * @param  intf_ptr Pointer to the interface specific data (I2C address)
 * @param  reg_addr Register address to write to
 * @param  data Pointer to the data buffer
 * @param  len Number of bytes to write
 * @retval BMP280_StatusTypeDef status of the operation
 */
BMP280_StatusTypeDef BMP280_I2C_Write(void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len);

/**
 * @brief  Reads data from BMP280 via I2C in non-blocking mode (Interrupt)
 * @param  intf_ptr Pointer to the interface specific data (I2C address)
 * @param  reg_addr Register address to read from
 * @param  data Pointer to the data buffer
 * @param  len Number of bytes to read
 * @retval BMP280_StatusTypeDef status of the operation
 */
BMP280_StatusTypeDef BMP280_I2C_Read_IT(void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len);

#endif