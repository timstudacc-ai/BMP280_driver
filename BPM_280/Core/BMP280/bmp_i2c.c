#include "bmp_i2c.h" // Підключаємо ваш заголовний файл!
#include "i2c.h"
//ffdhffgufgue
/* Згідно з bmp.h, функції мають повертати int8_t */

BMP280_StatusTypeDef BMP_I2C_Init(Bmp_280_Interface *bmp_device, void *device_address){

    if(bmp_device == NULL || device_address == NULL){
        return BMP280_ERR_I2C;
    }
    bmp_device->bus_read = BMP280_I2C_Read;
    bmp_device->bus_write = BMP280_I2C_Write;
    bmp_device->bus_read_IT = BMP280_I2C_Read_IT;
    bmp_device->intf_ptr = device_address;
    
    /* Відразу викликаємо головний драйвер */
    return BMP280_Init(bmp_device);
}

BMP280_StatusTypeDef BMP280_I2C_Read(void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    uint8_t dev_addr = (*(uint8_t*)intf_ptr);
    if(HAL_I2C_Mem_Read(&hi2c2, (dev_addr << 1U), reg_addr, 1, data, len, 100) == HAL_OK)
    {
        return  BMP280_OK; // Успіх
    }
    return BMP280_ERR_I2C; // Помилка
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

