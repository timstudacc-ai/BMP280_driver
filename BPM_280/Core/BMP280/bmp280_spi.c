#include "bmp280_spi.h"
#include "spi.h"
#include "bmp280.h"
#include <stdbool.h>

/**
 * @brief Global variable to hold the current CS pin for SPI transactions.
 *        Used to de-assert the CS pin asynchronously in DMA/IT callbacks.
 */
static void *current_cs_pin = NULL; 

/**
 * @brief Initializes the SPI interface for the BMP280.
 * @param bmp_device Pointer to the BMP280 driver interface structure.
 * @param CS_PIN Pointer to the SPI Chip Select pin structure.
 * @return BMP280_StatusTypeDef Status of the initialization (BMP280_OK if successful).
 */
BMP280_StatusTypeDef BMP280_SPI_Init(BMP280_Interface *bmp_device, void *handle, BMP280_SPI_CS_Pin *CS_PIN){
    /* Check for null pointers to prevent hard faults */
    if(bmp_device == NULL || CS_PIN == NULL){
        return BMP280_ERR_SPI;
    }
    
    /* Assign SPI specific read/write function pointers to the interface */
    bmp_device->bus_read = BMP280_SPI_Read;
    bmp_device->bus_write = BMP280_SPI_Write;
    bmp_device->bus_read_IT = BMP280_SPI_Read_IT;
    bmp_device->bus_read_DMA = BMP280_SPI_Read_DMA;
    bmp_device->bus_write_DMA = BMP280_SPI_Write_DMA;
    
    /* Store the CS pin hardware configuration into the interface pointer */
    bmp_device->intf_ptr = CS_PIN;
    bmp_device->handle = handle;
    
    /* Cache the CS pin globally for interrupt handlers */
    current_cs_pin = CS_PIN; 

    return BMP280_Init(bmp_device);
}

/**
 * @brief Initializes the Chip Select (CS) pin structure.
 * @param CS_PIN Pointer to the CS pin structure to initialize.
 * @param GPIOx GPIO port associated with the CS pin.
 * @param GPIO_Pin GPIO pin number.
 * @return BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_SPI_CS_Pin_Init(BMP280_SPI_CS_Pin *CS_PIN, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin){
    /* Validate parameters */
    if (CS_PIN == NULL || GPIOx == NULL) {
        return BMP280_ERR_NULL_PTR;
    }
    
    /* Assign port and pin to the structure */
    CS_PIN->GPIOx = GPIOx;
    CS_PIN->GPIO_Pin = GPIO_Pin;
    
    return BMP280_OK;
}

/**
 * @brief Starts an SPI transaction by asserting the CS pin (pulling it LOW).
 * @param intf_ptr Generic pointer to the BMP280_SPI_CS_Pin structure.
 * @return BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_SPI_Start_Transaction(void *intf_ptr){
    if (intf_ptr == NULL) {
        return BMP280_ERR_NULL_PTR;
    }
    
    BMP280_SPI_CS_Pin *cs_pin = (BMP280_SPI_CS_Pin *)intf_ptr;
    
    /* Pull CS low to activate the SPI slave device */
    HAL_GPIO_WritePin(cs_pin->GPIOx, cs_pin->GPIO_Pin, GPIO_PIN_RESET); 
    
    return BMP280_OK;
}

/**
 * @brief Ends an SPI transaction by de-asserting the CS pin (pulling it HIGH).
 * @param intf_ptr Generic pointer to the BMP280_SPI_CS_Pin structure.
 * @return BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_SPI_End_Transaction(void *intf_ptr){
    if (intf_ptr == NULL) {
        return BMP280_ERR_NULL_PTR;
    }
    
    BMP280_SPI_CS_Pin *cs_pin = (BMP280_SPI_CS_Pin *)intf_ptr;
    
    /* Pull CS high to deactivate the SPI slave device */
    HAL_GPIO_WritePin(cs_pin->GPIOx, cs_pin->GPIO_Pin, GPIO_PIN_SET); 
    
    return BMP280_OK;
}

/**
 * @brief Performs a blocking SPI write operation.
 * @param intf_ptr Pointer to the CS pin structure.
 * @param reg_addr Register address to write to.
 * @param data Pointer to the data payload to write.
 * @param len Length of the data payload in bytes.
 * @return BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_SPI_Write(void *handle, void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len){
    if (intf_ptr == NULL || data == NULL) {
        return BMP280_ERR_NULL_PTR;
    }
    
    /* Start the transaction */
    if (BMP280_SPI_Start_Transaction(intf_ptr) != BMP280_OK) {
        return BMP280_ERR_SPI;
    }
    
    /* Transmit the register address with the write bit (MSB = 0) */
    if (HAL_SPI_Transmit((SPI_HandleTypeDef*)handle, (uint8_t[]){BMP280_SPI_WRITE(reg_addr)}, 1, 100) != HAL_OK) {
        return BMP280_ERR_SPI;
    }
    
    /* Transmit the actual payload sequentially */
    if (HAL_SPI_Transmit((SPI_HandleTypeDef*)handle, data, len, 100) != HAL_OK) {
        return BMP280_ERR_SPI;
    }
    
    /* End the transaction */
    BMP280_SPI_End_Transaction(intf_ptr);
    return BMP280_OK;
}

/**
 * @brief Performs a blocking SPI read operation.
 * @param intf_ptr Pointer to the CS pin structure.
 * @param reg_addr Register address to read from.
 * @param data Pointer to the buffer to store received data.
 * @param len Length of data to read in bytes.
 * @return BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_SPI_Read(void *handle, void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len){
    if (intf_ptr == NULL || data == NULL) {
        return BMP280_ERR_NULL_PTR;
    }
    
    /* Start the transaction */
    if (BMP280_SPI_Start_Transaction(intf_ptr) != BMP280_OK) {
        return BMP280_ERR_SPI;
    }
    
    /* Transmit the register address with the read bit (MSB = 1) */
    if (HAL_SPI_Transmit((SPI_HandleTypeDef*)handle, (uint8_t[]){BMP280_SPI_READ(reg_addr)}, 1, 100) != HAL_OK) {
        return BMP280_ERR_SPI;
    }
    
    /* Receive the data payload sequentially */
    if (HAL_SPI_Receive((SPI_HandleTypeDef*)handle, data, len, 100) != HAL_OK) {
        return BMP280_ERR_SPI;
    }
    
    /* End the transaction */
    BMP280_SPI_End_Transaction(intf_ptr);
    return BMP280_OK;
}

/**
 * @brief Performs a non-blocking SPI read operation using Interrupts.
 * @param intf_ptr Pointer to the CS pin structure.
 * @param reg_addr Register address to read from.
 * @param data Pointer to the buffer to store received data.
 * @param len Length of data to read in bytes.
 * @return BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_SPI_Read_IT(void *handle, void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len){
    if (intf_ptr == NULL || data == NULL) {
        return BMP280_ERR_NULL_PTR;
    }
    
    /* Start the transaction */
    if (BMP280_SPI_Start_Transaction(intf_ptr) != BMP280_OK) {
        return BMP280_ERR_SPI;
    }
    
    /* Transmit the register address in blocking mode first */
    if (HAL_SPI_Transmit((SPI_HandleTypeDef*)handle, (uint8_t[]){BMP280_SPI_READ(reg_addr)}, 1, 100) != HAL_OK) {
        return BMP280_ERR_SPI;
    }
    
    /* Initiate the non-blocking receive via Interrupts */
    if (HAL_SPI_Receive_IT((SPI_HandleTypeDef*)handle, data, len) != HAL_OK) {
        return BMP280_ERR_SPI;
    }
    
    /* The transaction will be ended asynchronously in the Rx Complete callback */
    return BMP280_OK;
}

/**
 * @brief Performs a non-blocking SPI read operation using DMA.
 * @param intf_ptr Pointer to the CS pin structure.
 * @param reg_addr Register address to read from.
 * @param data Pointer to the buffer to store received data.
 * @param len Length of data to read in bytes.
 * @return BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_SPI_Read_DMA(void *handle, void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len){
    if (intf_ptr == NULL || data == NULL) {
        return BMP280_ERR_NULL_PTR;
    }
    
    /* Start the transaction */
    if (BMP280_SPI_Start_Transaction(intf_ptr) != BMP280_OK) {
        return BMP280_ERR_SPI;
    }
    
    /* Transmit the register address in blocking mode to prepare the bus */
    if (HAL_SPI_Transmit((SPI_HandleTypeDef*)handle, (uint8_t[]){BMP280_SPI_READ(reg_addr)}, 1,100) != HAL_OK) {
        return BMP280_ERR_SPI;
    }
    
    /* Initiate the fast non-blocking receive via DMA */
    if (HAL_SPI_Receive_DMA((SPI_HandleTypeDef*)handle, data, len) != HAL_OK) {
        return BMP280_ERR_SPI;
    }
    
    /* The transaction will be ended asynchronously in the Rx Complete callback */
    return BMP280_OK;
}

/**
 * @brief Performs a non-blocking SPI write operation using DMA.
 * @param intf_ptr Pointer to the CS pin structure.
 * @param reg_addr Register address to write to.
 * @param data Pointer to the data payload to write.
 * @param len Length of the data payload in bytes.
 * @return BMP280_StatusTypeDef Status of the operation.
 */
BMP280_StatusTypeDef BMP280_SPI_Write_DMA(void *handle, void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len){
    if (intf_ptr == NULL || data == NULL) {
        return BMP280_ERR_NULL_PTR;
    }
    
    /* Start the transaction */
    if (BMP280_SPI_Start_Transaction(intf_ptr) != BMP280_OK) {
        return BMP280_ERR_SPI;
    }
    
    /* Transmit the register address in blocking mode */
    if (HAL_SPI_Transmit((SPI_HandleTypeDef*)handle, (uint8_t[]){BMP280_SPI_WRITE(reg_addr)}, 1,100) != HAL_OK) {
        return BMP280_ERR_SPI;
    }
    
    /* Initiate the fast non-blocking write via DMA */
    if (HAL_SPI_Transmit_DMA((SPI_HandleTypeDef*)handle, data, len) != HAL_OK) {
        return BMP280_ERR_SPI;
    }
    
    /* The transaction will be ended asynchronously in the Tx Complete callback */
    return BMP280_OK;
}

/**
 * @brief HAL SPI Tx Complete Callback. Called automatically when DMA/IT transmission finishes.
 * @param hspi Pointer to the SPI handle.
 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    /* Pull CS high to end the transaction */
    if (current_cs_pin != NULL) {
        BMP280_SPI_End_Transaction(current_cs_pin);
    }
}

/**
 * @brief HAL SPI Rx Complete Callback. Called automatically when DMA/IT reception finishes.
 * @param hspi Pointer to the SPI handle.
 */
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    /* Only process callbacks for our target SPI bus */
    if (hspi->Instance == SPI1)
    {
        /* Pull CS high to end the transaction */
        if (current_cs_pin != NULL) {
            BMP280_SPI_End_Transaction(current_cs_pin);
            
            /* Notify the BMP280 driver state machine that data is ready */
            BMP280_RX_Cplt_Callback();
        }
    }
}
