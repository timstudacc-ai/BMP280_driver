#include "uart_dma_manager.h"
#include "dma.h"
#include "main.h" /* For GPIO definitions */
#include <stdbool.h>

/* Encapsulated static state for the UART DMA Manager */
static UART_HandleTypeDef *p_huart = NULL; /* Pointer to the UART hardware handle */
static RingBuffer *p_rx_buf = NULL;        /* High-level software ring buffer for reception */
static RingBuffer *p_tx_buf = NULL;        /* High-level software ring buffer for transmission */

/* Hardware DMA Buffers */
#define BUFFER_SIZE 128
static uint8_t rx_dma_buf[BUFFER_SIZE]; /* Circular DMA buffer for continuous background reception */

/**
 * @brief Ping-Pong transmission buffers.
 * @details Enables 100% bus utilization by allowing CPU preparation of one buffer
 *          while DMA transmits the other. The 128-byte size optimizes SRAM usage;
 *          larger payloads are automatically fragmented and later reassembled by
 *          the protocol layer. These buffers provide the necessary contiguous linear
 *          memory required by the hardware DMA to bypass ring buffer wrap-arounds.
 */
static uint8_t tx_buf_A[BUFFER_SIZE]; /* Ping-Pong TX Buffer A */
static uint8_t tx_buf_B[BUFFER_SIZE]; /* Ping-Pong TX Buffer B */

/* State variables for RX Circular Extraction */
static uint16_t rx_old_pos = 0; /* Tracks the last read position in rx_dma_buf */

/* State variables for TX Ping-Pong Operation */
static volatile bool tx_dma_busy = false;  /* Flag indicating if the TX DMA is currently transmitting */
static volatile uint8_t tx_active_buf = 0; /* Indicates which ping-pong buffer is currently held by DMA (0 = A, 1 = B) */

/* Software Flow Control State */
static bool sw_flow_control_enabled = false;

/* Flow Control Watermarks (Hysteresis) */
#define RX_BUFFER_WATERMARK_HIGH (RING_BUFFER_SIZE * 0.8)
#define RX_BUFFER_WATERMARK_LOW (RING_BUFFER_SIZE * 0.2)
#define TX_BUFFER_WATERMARK_HIGH (RING_BUFFER_SIZE * 0.8)
#define TX_BUFFER_WATERMARK_LOW (RING_BUFFER_SIZE * 0.2)

/**
 * @brief Enable or disable software manual flow control.
 * @param enable true to enable flow control, false to disable.
 */
void UART_Manager_EnableSoftwareFlowControl(bool enable)
{
    sw_flow_control_enabled = enable;
}

/**
 * @brief  Initializes the UART DMA Manager.
 * @param  huart: Pointer to the UART handle
 * @param  rx_ptr: Pointer to the RX software ring buffer
 * @param  tx_ptr: Pointer to the TX software ring buffer
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef UART_Manager_Init(UART_HandleTypeDef *huart, RingBuffer *rx_ptr, RingBuffer *tx_ptr)
{
    if (huart == NULL || rx_ptr == NULL || tx_ptr == NULL)
    {
        return HAL_ERROR;
    }

    p_huart = huart;
    p_rx_buf = rx_ptr;
    p_tx_buf = tx_ptr;

    rx_old_pos = 0;
    return HAL_UARTEx_ReceiveToIdle_DMA(p_huart, rx_dma_buf, sizeof(rx_dma_buf));
}

/**
 * @brief  Main Task for UART Manager.
 *         Responsible for safely pulling data from the software TX Ring Buffer
 *         and triggering the hardware DMA transmission.
 *         Must be called continuously in the main application loop.
 */
void UART_Manager_Task(void)
{
    if (p_huart == NULL || p_rx_buf == NULL || p_tx_buf == NULL)
    {
        return;
    }

    /* 1. Software Flow Control */
    if (sw_flow_control_enabled)
    {
        uint16_t rx_count = rb_get_count(p_rx_buf);
        if (rx_count >= RX_BUFFER_WATERMARK_HIGH)
        {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET); /* Assert RTS HIGH (Stop) */
        }
        else if (rx_count <= RX_BUFFER_WATERMARK_LOW)
        {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET); /* Assert RTS LOW (Ready) */
        }

        uint16_t tx_count = rb_get_count(p_tx_buf);
        if (tx_count >= TX_BUFFER_WATERMARK_HIGH)
        {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);
        }
        else if (tx_count <= TX_BUFFER_WATERMARK_LOW)
        {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
        }
    }

    /* 2. DMA TX Kick-off */

    /**
     * @note CRITICAL SECTION
     * Interrupts are disabled to ensure atomic access to the STM32 HAL state and __HAL_LOCK.
     * - USART1_IRQn: Prevents RX events from causing race conditions on the huart state.
     * - DMA2_Stream7_IRQn: Prevents premature triggering of TxCpltCallback for short
     *   payloads, which would otherwise result in a HAL_BUSY lock collision.
     */
    NVIC_DisableIRQ(DMA2_Stream7_IRQn);
    NVIC_DisableIRQ(USART1_IRQn);
    if (!tx_dma_busy && !rb_is_empty(p_tx_buf))
    {
        /**
         * @note Memory clearing (e.g., memset) is intentionally omitted to optimize
         * CPU cycles
         . The DMA transfer is strictly bounded by `tx_len`.
         */
        uint16_t tx_len = rb_pop_array(p_tx_buf, tx_buf_A, sizeof(tx_buf_A));
        if (tx_len > 0)
        {
            tx_dma_busy = true;
            tx_active_buf = 0;
            /* Initiate DMA transfer restricted to the exact payload length */
            HAL_UART_Transmit_DMA(p_huart, tx_buf_A, tx_len);
        }
    }
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_EnableIRQ(DMA2_Stream7_IRQn);
}

/**
 * @brief  RX Event Callback triggered by the HAL library.
 *         This function is called primarily when the physical UART line goes IDLE,
 *         or when the circular DMA buffer reaches exactly half or full capacity.
 * @param  huart: Pointer to the UART handle
 * @param  Size: The absolute position (index) of the DMA write pointer within rx_dma_buf
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (p_huart != NULL && huart->Instance == p_huart->Instance)
    {
        uint16_t new_pos = Size;

        if (new_pos != rx_old_pos)
        {
            if (new_pos > rx_old_pos)
            {
                uint16_t len = new_pos - rx_old_pos;
                rb_push_array(p_rx_buf, &rx_dma_buf[rx_old_pos], len);
            }
            else
            {
                /* Wrap-around detected */
                uint16_t len1 = sizeof(rx_dma_buf) - rx_old_pos;
                if (len1 > 0)
                {
                    rb_push_array(p_rx_buf, &rx_dma_buf[rx_old_pos], len1);
                }
                if (new_pos > 0)
                {
                    rb_push_array(p_rx_buf, rx_dma_buf, new_pos);
                }
            }

            /* Edge case: If DMA fills the exact end of the buffer, Size (new_pos) will be 128.
             * Since valid indices are 0 to 127, we must wrap rx_old_pos back to 0 manually. */
            rx_old_pos = new_pos;
            if (rx_old_pos >= sizeof(rx_dma_buf))
            {
                rx_old_pos = 0;
            }
        }

        /**
         * @note Workaround for STM32 HAL IDLE state anomaly.
         * If the HAL framework prematurely sets the state to READY, it is manually
         * reverted to BUSY_RX and the IDLE interrupt is re-enabled to prevent
         * data loss during continuous Circular DMA reception.
         */
        if (huart->RxState == HAL_UART_STATE_READY)
        {
            huart->RxState = HAL_UART_STATE_BUSY_RX;
            SET_BIT(huart->Instance->CR1, USART_CR1_IDLEIE);
        }
    }
}

/**
 * @brief  TX Complete Callback triggered when the DMA finishes transmitting a buffer.
 *         Implements Ping-Pong continuous transmission. If more data exists in the
 *         software TX Ring Buffer, it instantly loads it into the alternate ping-pong
 *         buffer and restarts the DMA without waiting for the main loop.
 * @param  huart: Pointer to the UART handle
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (p_huart != NULL && huart->Instance == p_huart->Instance)
    {
        if (tx_active_buf == 0)
        {
            uint16_t len = rb_pop_array(p_tx_buf, tx_buf_B, sizeof(tx_buf_B));
            if (len > 0)
            {
                tx_active_buf = 1;
                HAL_UART_Transmit_DMA(p_huart, tx_buf_B, len);
            }
            else
            {
                tx_dma_busy = false;
            }
        }
        else
        {
            uint16_t len = rb_pop_array(p_tx_buf, tx_buf_A, sizeof(tx_buf_A));
            if (len > 0)
            {
                tx_active_buf = 0;
                HAL_UART_Transmit_DMA(p_huart, tx_buf_A, len);
            }
            else
            {
                tx_dma_busy = false;
            }
        }
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (p_huart != NULL && huart->Instance == p_huart->Instance)
    {

        HAL_UART_AbortReceive(p_huart);
        rx_old_pos = 0;
        HAL_UARTEx_ReceiveToIdle_DMA(p_huart, rx_dma_buf, sizeof(rx_dma_buf));
    }
}
