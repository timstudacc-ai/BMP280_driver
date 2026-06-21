
#include "bmp_280_SPI.h"
#include "spi.h"
#include "bmp.h"
#include <stdbool.h>

static void *current_cs_pin = NULL; /* Global variable to hold the current CS pin for SPI transactions */



BMP280_StatusTypeDef BMP280_SPI_Init(Bmp_280_Interface *bmp_device, SPI_BMP_CS_PIN *CS_PIN){
  if(bmp_device == NULL || CS_PIN == NULL){
        return BMP280_ERR_SPI;
    }
    bmp_device->bus_read = BMP280_SPI_Read;
    bmp_device->bus_write = BMP280_SPI_Write;
    bmp_device->bus_read_IT = BMP280_SPI_Read_IT;
    bmp_device->bus_read_DMA = BMP280_SPI_Read_DMA;
    bmp_device->bus_write_DMA = BMP280_SPI_Write_DMA;
    bmp_device->intf_ptr = CS_PIN;
    current_cs_pin = CS_PIN; /* Store the CS pin for use in callbacks */

 
    
    /* Call the main driver initialization immediately */
    return BMP280_OK;
}
BMP280_StatusTypeDef BMP280_CS_PIN_INIT(SPI_BMP_CS_PIN *CS_PIN, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin){
    if (CS_PIN == NULL || GPIOx == NULL) {
        return BMP280_ERR_NULL_PTR;
    }
    CS_PIN->GPIOx = GPIOx;
    CS_PIN->GPIO_Pin = GPIO_Pin;
    return BMP280_OK;
}
BMP280_StatusTypeDef BMP280_SPI_START_TRANSACTION(void *intf_ptr){
    if (intf_ptr == NULL) {
        return BMP280_ERR_NULL_PTR;
    }
        SPI_BMP_CS_PIN *cs_pin = (SPI_BMP_CS_PIN *)intf_ptr;
        HAL_GPIO_WritePin(cs_pin->GPIOx, cs_pin->GPIO_Pin, GPIO_PIN_RESET); /* Assert CS (active low) */
    
    return BMP280_OK;
}
BMP280_StatusTypeDef BMP280_SPI_END_TRANSACTION(void *intf_ptr){
    if (intf_ptr == NULL) {
        return BMP280_ERR_NULL_PTR;
    }
    SPI_BMP_CS_PIN *cs_pin = (SPI_BMP_CS_PIN *)intf_ptr;
    HAL_GPIO_WritePin(cs_pin->GPIOx, cs_pin->GPIO_Pin, GPIO_PIN_SET); /* Deassert CS (inactive high) */
    return BMP280_OK;
}
BMP280_StatusTypeDef BMP280_SPI_Write(void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len){
    if (intf_ptr == NULL || data == NULL) {
        return BMP280_ERR_NULL_PTR;
    }
    if (BMP280_SPI_START_TRANSACTION(intf_ptr) != BMP280_OK) {
        return BMP280_ERR_SPI;
    }
    if (HAL_SPI_Transmit(&hspi1, (uint8_t[]){BMP280_SPI_WRITE(reg_addr)}, 1, 100) != HAL_OK) {
        return BMP280_ERR_SPI;
    }
    if (HAL_SPI_Transmit(&hspi1, data, len, 100) != HAL_OK) {
        return BMP280_ERR_SPI;
    }
    BMP280_SPI_END_TRANSACTION(intf_ptr);
    return BMP280_OK;
}
BMP280_StatusTypeDef BMP280_SPI_Read(void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len){
    if (intf_ptr == NULL || data == NULL) {
        return BMP280_ERR_NULL_PTR;
    }
    if (BMP280_SPI_START_TRANSACTION(intf_ptr) != BMP280_OK) {
        return BMP280_ERR_SPI;
    }
    if (HAL_SPI_Transmit(&hspi1, (uint8_t[]){BMP280_SPI_READ(reg_addr)}, 1, 100) != HAL_OK) {
        return BMP280_ERR_SPI;
    }
    if (HAL_SPI_Receive(&hspi1, data, len, 100) != HAL_OK) {
        return BMP280_ERR_SPI;
    }
    BMP280_SPI_END_TRANSACTION(intf_ptr);
    return BMP280_OK;
}
BMP280_StatusTypeDef BMP280_SPI_Read_IT(void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len){
    if (intf_ptr == NULL || data == NULL) {
        return BMP280_ERR_NULL_PTR;
    }
    if (BMP280_SPI_START_TRANSACTION(intf_ptr) != BMP280_OK) {
        return BMP280_ERR_SPI;
    }
    if (HAL_SPI_Transmit(&hspi1, (uint8_t[]){BMP280_SPI_READ(reg_addr)}, 1, 100) != HAL_OK) {
        return BMP280_ERR_SPI;
    }
    if (HAL_SPI_Receive_IT(&hspi1, data, len) != HAL_OK) {
        return BMP280_ERR_SPI;
    }
    /* The transaction will be ended in the Rx Complete callback */
    return BMP280_OK;
}

BMP280_StatusTypeDef BMP280_SPI_Read_DMA(void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len){
    if (intf_ptr == NULL || data == NULL) {
        return BMP280_ERR_NULL_PTR;
    }
    if (BMP280_SPI_START_TRANSACTION(intf_ptr) != BMP280_OK) {
        return BMP280_ERR_SPI;
    }
    if (HAL_SPI_Transmit(&hspi1, (uint8_t[]){BMP280_SPI_READ(reg_addr)}, 1,100) != HAL_OK) {
        return BMP280_ERR_SPI;
    }
    if (HAL_SPI_Receive_DMA(&hspi1, data, len) != HAL_OK) {
        return BMP280_ERR_SPI;
    }
    /* The transaction will be ended in the Rx Complete callback */
    return BMP280_OK;
}

BMP280_StatusTypeDef BMP280_SPI_Write_DMA(void *intf_ptr, uint8_t reg_addr, uint8_t *data, uint16_t len){
    if (intf_ptr == NULL || data == NULL) {
        return BMP280_ERR_NULL_PTR;
    }
    if (BMP280_SPI_START_TRANSACTION(intf_ptr) != BMP280_OK) {
        return BMP280_ERR_SPI;
    }
    if (HAL_SPI_Transmit(&hspi1, (uint8_t[]){BMP280_SPI_WRITE(reg_addr)}, 1,100) != HAL_OK) {
        return BMP280_ERR_SPI;
    }
    if (HAL_SPI_Transmit_DMA(&hspi1, data, len) != HAL_OK) {
        return BMP280_ERR_SPI;
    }
    /* The transaction will be ended in the Tx Complete callback */
    return BMP280_OK;
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (current_cs_pin != NULL) {
    BMP280_SPI_END_TRANSACTION(current_cs_pin);
  }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI1)
  {
    if (current_cs_pin != NULL) {
    BMP280_SPI_END_TRANSACTION(current_cs_pin);
    BMP280_Rx_CpltCallback();
    }
  }
}