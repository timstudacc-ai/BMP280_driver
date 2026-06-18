/**
 * @file bmp_i2c.c
 * @brief I2C porting layer implementation for BMP280 sensor
 */
#include "bmp_I2c.h" 
#include "i2c.h"
// added my own shit and remoced other shit
/* According to bmp.h, read/write functions must match the typedef signatures */

BMP280_StatusTypeDef BMP_I2C_Init(Bmp_280_Interface *bmp_device, void *device_address){

    if(bmp_device == NULL || device_address == NULL){
        return BMP280_ERR_I2C;
    }
    bmp_device->bus_read = BMP280_I2C_Read;
    bmp_device->bus_write = BMP280_I2C_Write;
    bmp_device->bus_read_IT = BMP280_I2C_Read_IT;
    bmp_device->intf_ptr = device_address;
    
    /* Call the main driver initialization immediately */
    return BMP280_Init(bmp_device);
}

BMP280_StatusTypeDef BMP280_I2C_Read(void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    uint8_t dev_addr = (*(uint8_t*)intf_ptr);
    if(HAL_I2C_Mem_Read(&hi2c2, (dev_addr << 1U), reg_addr, 1, data, len, 100) == HAL_OK)
    {
        return  BMP280_OK; /* Success */
    }
    return BMP280_ERR_I2C; /* Error */
}

BMP280_StatusTypeDef BMP280_I2C_Write(void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    uint8_t dev_addr = (*(uint8_t*)intf_ptr);
    if(HAL_I2C_Mem_Write(&hi2c2, (dev_addr << 1U), reg_addr, 1, data, len, 100) == HAL_OK)
    {
        return  BMP280_OK;
    }
    return BMP280_ERR_I2C;
}

BMP280_StatusTypeDef BMP280_I2C_Read_IT(void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    uint8_t dev_addr = (*(uint8_t*)intf_ptr);
    if(HAL_I2C_Mem_Read_IT(&hi2c2, (dev_addr << 1U), reg_addr, 1, data, len) == HAL_OK)
    {
        return  BMP280_OK;
    }
    return BMP280_ERR_I2C;
}

