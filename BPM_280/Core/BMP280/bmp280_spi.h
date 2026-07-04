#ifndef _SPI_BMP280_H_
#define _SPI_BMP280_H_

#include "bmp280.h"
#include "spi.h"

/**
 * @brief Macro to set the MSB for SPI read operations.
 *        The BMP280 requires the MSB of the register address to be 1 for a read.
 */
#define BMP280_SPI_READ(REG) ((REG) | 0x80U)

/**
 * @brief Macro to clear the MSB for SPI write operations.
 *        The BMP280 requires the MSB of the register address to be 0 for a write.
 */
#define BMP280_SPI_WRITE(REG) ((REG) & 0x7FU)

/**
 * @brief Structure to define the Chip Select (CS) pin for the SPI interface.
 */
typedef struct
{
    GPIO_TypeDef *GPIOx; /**< GPIO port for the CS pin */
    uint16_t GPIO_Pin;   /**< GPIO pin number for the CS pin */
} BMP280_SPI_CS_Pin;

/**
 * @brief Initializes the SPI interface for the BMP280 sensor.
 * @param bmp_device Pointer to the BMP280 interface structure.
 * @param CS_PIN Pointer to the CS pin configuration structure.
 * @return BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_SPI_Init(BMP280_Interface *bmp_device, void *handle, BMP280_SPI_CS_Pin *CS_PIN);

/**
 * @brief Performs a blocking SPI read operation.
 * @param intf_ptr Pointer to the interface specific data (e.g., CS_PIN).
 * @param reg_addr Register address to read from.
 * @param data Pointer to the buffer where received data will be stored.
 * @param len Number of bytes to read.
 * @return BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_SPI_Read(void *handle, void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len);

/**
 * @brief Performs a blocking SPI write operation.
 * @param intf_ptr Pointer to the interface specific data (e.g., CS_PIN).
 * @param reg_addr Register address to write to.
 * @param data Pointer to the buffer containing data to be written.
 * @param len Number of bytes to write.
 * @return BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_SPI_Write(void *handle, void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len);

/**
 * @brief Performs a non-blocking SPI read operation using Interrupts.
 * @param intf_ptr Pointer to the interface specific data (e.g., CS_PIN).
 * @param reg_addr Register address to read from.
 * @param data Pointer to the buffer where received data will be stored.
 * @param len Number of bytes to read.
 * @return BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_SPI_Read_IT(void *handle, void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len);

/**
 * @brief Performs a non-blocking SPI read operation using DMA.
 * @param intf_ptr Pointer to the interface specific data (e.g., CS_PIN).
 * @param reg_addr Register address to read from.
 * @param data Pointer to the buffer where received data will be stored.
 * @param len Number of bytes to read.
 * @return BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_SPI_Read_DMA(void *handle, void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len);

/**
 * @brief Performs a non-blocking SPI write operation using DMA.
 * @param intf_ptr Pointer to the interface specific data (e.g., CS_PIN).
 * @param reg_addr Register address to write to.
 * @param data Pointer to the buffer containing data to be written.
 * @param len Number of bytes to write.
 * @return BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_SPI_Write_DMA(void *handle, void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len);

/**
 * @brief Initializes the CS pin structure.
 * @param CS_PIN Pointer to the CS pin configuration structure.
 * @param GPIOx GPIO port for the CS pin.
 * @param GPIO_Pin GPIO pin number for the CS pin.
 * @return BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_SPI_CS_Pin_Init(BMP280_SPI_CS_Pin *CS_PIN, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

/**
 * @brief Asserts the CS pin (pulls it low) to start an SPI transaction.
 * @param intf_ptr Pointer to the interface specific data (e.g., CS_PIN).
 * @return BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_SPI_Start_Transaction(void *intf_ptr);

/**
 * @brief De-asserts the CS pin (pulls it high) to end an SPI transaction.
 * @param intf_ptr Pointer to the interface specific data (e.g., CS_PIN).
 * @return BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_SPI_End_Transaction(void *intf_ptr);    

#endif /* _SPI_BMP280_H_ */