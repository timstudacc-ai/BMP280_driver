#ifndef _SPI_BMP280_H_
#define _SPI_BMP280_H_

#include "bmp.h"
#include "spi.h"
#define BMP280_SPI_READ(REG) ((REG) | 0x80U)  /* MSB set for read operations */
#define BMP280_SPI_WRITE(REG) ((REG) & 0x7FU) /* MSB cleared for write operations */

typedef struct SPI_BMP_CS_PIN
{
    GPIO_TypeDef *GPIOx; /* GPIO port for the CS pin */
    uint16_t GPIO_Pin;   /* GPIO pin number for the CS pin */
} SPI_BMP_CS_PIN;

BMP280_StatusTypeDef BMP280_SPI_Init(Bmp_280_Interface *bmp_device, SPI_BMP_CS_PIN *CS_PIN);

BMP280_StatusTypeDef BMP280_SPI_Read(void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len);

BMP280_StatusTypeDef BMP280_SPI_Write(void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len);

BMP280_StatusTypeDef BMP280_SPI_Read_IT(void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len);

BMP280_StatusTypeDef BMP280_CS_PIN_INIT(SPI_BMP_CS_PIN *CS_PIN, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

BMP280_StatusTypeDef BMP280_SPI_START_TRANSACTION(void *intf_ptr);

BMP280_StatusTypeDef BMP280_SPI_END_TRANSACTION(void *intf_ptr);    

#endif /* _SPI_BMP280_H_ */