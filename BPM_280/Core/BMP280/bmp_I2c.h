#ifndef _BMP_I2C_H
#define _BMP_I2C_H


#include "bmp.h"
BMP280_StatusTypeDef BMP_I2C_Init(Bmp_280_Interface *bmp_device, void *device_address);
BMP280_StatusTypeDef BMP280_I2C_Read(void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len);
BMP280_StatusTypeDef BMP280_I2C_Write(void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len);
BMP280_StatusTypeDef BMP280_I2C_Read_IT(void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len);


#endif