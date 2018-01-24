/*
 * uartbuftx.c
 *
 *  Created on: 08.11.2017
 *      Author: Kamil Cukrowski
 *     License: jointly under MIT License and Beerware License
 */
#include "uartbuftx.h"

#include "_uartbuf.h"

#include <string.h>
#include <assert.h>
#include <stdio.h>

/* Private Macros ------------------------------------------------------- */

#define uartbuftx_ATOMIC_BLOCK(t) \
	for( bool _ToDo = (uartbuftx_DisableIRQ_Callback(t), true); \
	_ToDo; \
	_ToDo = (uartbuftx_EnableIRQ_Callback(t), false) )

#if SIZE_T_ATOMIC_READ
# define uartbuftx_SIZE_T_ATOMIC_BLOCK(t) /**/
#else
# define uartbuftx_SIZE_T_ATOMIC_BLOCK(t) uartbuftx_ATOMIC_BLOCK(t)
#endif

/* Private Functions ----------------------------------------------------- */

static
size_t uartbuftx_fifo_tail(const struct uartbuftx_s *t)
{
	size_t ret;
	uartbuftx_SIZE_T_ATOMIC_BLOCK(t) {
		ret = t->tail;
	}
	return ret;
}

static inline
size_t uartbuftx_fifo_GetBiggestUsedBlock_IRQHandler(const struct uartbuftx_s *t)
{
	const size_t head = t->head;
	const size_t tail = t->tail;
	const size_t size = t->size;
	size_t ret;
	if ( tail > head ) {
		ret = size - tail;
	} else {
		ret = head - tail;
	}
	UARTBUF_DBG("%s %zu %zu %zu %zu\n", __func__, head, tail, size, ret);
	return ret;
}

static inline
size_t uartbuftx_fifo_GetBiggestFreeBlock(const struct uartbuftx_s *t)
{
	const size_t head = t->head;
	const size_t tail = uartbuftx_fifo_tail(t);
	const size_t size = t->size;
	size_t ret;
	if ( tail > head ) {
		ret = tail - head - 1;
	} else if ( tail == 0 ) {
		ret = size - head - 1;
	} else {
		ret = size - head;
	}
	UARTBUF_DBG("%s %zu %zu %zu %zu\n", __func__, head, tail, size, ret);
	return ret;
}

static inline
size_t uartbuftx_fifo_IncPos(const struct uartbuftx_s *t, size_t pos, size_t n)
{
	unitassert(pos + n <= t->size);
	pos += n;
	if ( pos == t->size ) {
		pos = 0;
	}
	return pos;
}

/* Public Functions -------------------------------------------------------- */

inline
bool uartbuftx_IsWriting(const struct uartbuftx_s *t)
{
	return uartbuftx_IsWriting_Callback(t);
}

/**
 * Calls Flush_Callback.
 * @param t
 */
void uartbuftx_Flush(struct uartbuftx_s *t)
{
	while ( uartbuftx_IsWriting(t) ) {
		uartbuftx_Flush_Callback(t);
	}
}

void uartbuftx_Write(struct uartbuftx_s *t, const uint8_t buf[restrict], size_t size)
{
	if ( size && ( !t->buf || t->size < 1 ) ) {
		uartbuftx_Write_Callback(t, buf, size);
		size = 0;
	}
	if ( !size ) {
		uartbuftx_Flush(t);
		return;
	}
	while( size ) {
		// get biggest block we can copy into fifo
		const size_t free = uartbuftx_fifo_GetBiggestFreeBlock(t);
		if ( free == 0 ) {
			uartbuftx_Flush_Callback(t);
			continue;
		}

		const size_t len = size > free ? free : size;
		// copy data up to len bytes
		memcpy(&t->buf[t->head], &buf[0], len);
		// increment head pointer
		{
			const size_t newhead = uartbuftx_fifo_IncPos(t, t->head, len);
			uartbuftx_SIZE_T_ATOMIC_BLOCK(t) {
				t->head = newhead;
			}
		}
		// rearm writing if ended
		if ( !uartbuftx_IsWriting(t) ) {
			// we may call IRQHandler from here, cause IRQ is not armed, as we are not writing
			const size_t txsize = uartbuftx_fifo_GetBiggestUsedBlock_IRQHandler(t);
			uartbuftx_Write_Callback(t, &t->buf[t->tail], txsize);
		}

		size -= len;
		buf += len;
	}
}

void uartbuftx_printf(struct uartbuftx_s *t)
{
	printf("head=%2zu size=%2zu tail=%2zu\"",
			t->head, t->size, uartbuftx_fifo_tail(t));
	for(size_t i = 0; i < t->size; ++i) {
		printf("%c", t->buf[i]);
	}
	printf("\"\n");
}

void uartbuftx_TxCplt_IRQHandler(struct uartbuftx_s *t, size_t size)
{
	if ( !t->size ) return;
	assert( size <= uartbuftx_fifo_GetBiggestUsedBlock_IRQHandler(t) );
	t->tail = uartbuftx_fifo_IncPos(t, t->tail, size);
	const size_t txsize = uartbuftx_fifo_GetBiggestUsedBlock_IRQHandler(t);
	uartbuftx_Write_IRQHandler_Callback(t, &t->buf[t->tail], txsize);
}


/* Callback Functions -------------------------------------------------------- */

/**
 * Enable uartbuftx_RxCplt_IRQHandler to be called. Leave critical section.
 * @param t
 */
__weak_symbol
void uartbuftx_EnableIRQ_Callback(const struct uartbuftx_s *t)
{
#if UARTBUFTX_USE_PNT_CALLBACKS
	if ( t->EnableIRQ ) t->EnableIRQ(t);
#endif // UARTBUFTX_USE_PNT_CALLBACKS
	// NVIC_Enable_IRQ(IRQ_UART2);
}

/**
 * Dissallow RxCplt_IRQHandler to be called. Enter critical section.
 * @param t
 */
__weak_symbol
void uartbuftx_DisableIRQ_Callback(const struct uartbuftx_s *t)
{
#if UARTBUFTX_USE_PNT_CALLBACKS
	if ( t->DisableIRQ ) t->DisableIRQ(t);
#endif // UARTBUFTX_USE_PNT_CALLBACKS
	// NVIC_Disable_IRQ(IRQ_UART2);
}

__weak_symbol
bool uartbuftx_IsWriting_Callback(const struct uartbuftx_s *t)
{
	return false;
}

/**
 * Write size bytes starting from buf to uart
 * @param t
 * @param buf
 * @param size
 */
__weak_symbol
void uartbuftx_Write_Callback(struct uartbuftx_s *t, const uint8_t buf[], size_t size)
{
#if UARTBUFTX_USE_PNT_CALLBACKS
	if ( t->Write ) t->Write(t, buf, size);
#endif // UARTBUFTX_USE_PNT_CALLBACKS
	// if ( HAL_UART_Transmit_DMA(t->priv.huart, buf, size, 100) != HAL_OK ) assert(0);
	// or
	// if ( HAL_UART_Transmit(t->priv.huart, buf, size, 100) != HAL_OK ) assert(0);
	// uartbuftx_TxCplt_IRQHandler((struct uartbuftx_s *)t);
}

/**
 * Same as uartbuftx_Write_Callback but called from IRQHandler
 * @param t
 * @param buf
 * @param size
 */
__weak_symbol
void uartbuftx_Write_IRQHandler_Callback(struct uartbuftx_s *t, const uint8_t buf[], size_t size)
{
#if UARTBUFTX_USE_PNT_CALLBACKS
	if ( t->Write_IRQHandler ) t->Write_IRQHandler(t, buf, size);
#else
	uartbuftx_Write_Callback(t, buf, size);
#endif // UARTBUFTX_USE_PNT_CALLBACKS
	// if ( size == 0 ) return;
	// if ( HAL_UART_Transmit_DMA(t->priv.huart, buf, size, 100) != HAL_OK ) assert(0);
}

/**
 * Wait until currently asynchronous tranmission is finished.
 * This should wait until TxCplt_IRQHandler is called.
 * @param t
 */
__weak_symbol
void uartbuftx_Flush_Callback(const struct uartbuftx_s *t)
{
#if UARTBUFTX_USE_PNT_CALLBACKS
	if ( t->Flush ) t->Flush(t);
#endif // UARTBUFTX_USE_PNT_CALLBACKS
	// BoardEnterSleepModeToPreserveEnergyAndWakeUpOnUsartInterrupt();
}
