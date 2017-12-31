/*
 * uartbuftx.c
 *
 *  Created on: 08.11.2017
 *      Author: Kamil Cukrowski
 *     License: jointly under MIT License and Beerware License
 */
#include "_uartbuf.h"
#include "uartbuftx.h"

#include <string.h>
#include <assert.h>

/* Private Macros ------------------------------------------------------- */

#define uartbuftx_ATOMIC_BLOCK(t) \
	for( bool _ToDo = (uartbuftx_DisableIRQ_Callback(t), true); \
	_ToDo; \
	_ToDo = (uartbuftx_EnableIRQ_Callback(t), false) )

/* Private Functions ----------------------------------------------------- */

static inline
bool _uartbuftx_IsWriting(struct uartbuftx_s *t)
{
	return t->nowtxpos;
}


/* Callback Functions -------------------------------------------------------- */

#ifdef __weak_symbol

__weak_symbol
void uartbuftx_EnableIRQ_Callback(const struct uartbuftx_s *t)
{
	// Enable TxCplt_IRQHndler from occuring
	// NVIC_Enable_IRQ(IRQ_UART2);
}

__weak_symbol
void uartbuftx_DisableIRQ_Callback(const struct uartbuftx_s *t)
{
	// Disable TxCplt_IRQHndler from occuring
	// NVIC_Disable_IRQ(IRQ_UART2);
}

__weak_symbol
void uartbuftx_Write_Callback(const struct uartbuftx_s *t, const uint8_t buf[], size_t size)
{
	// set up asynchronous write of buf and size characters
	// if ( HAL_UART_Transmit_DMA(t->priv.huart, buf, size, 100) != HAL_OK ) assert(0);
}

__weak_symbol
void uartbuftx_Flush_Callback(const struct uartbuftx_s *t)
{
	// wait until current asynchronous transmission is finished
	// while( !t->priv.huart.state != HAL_UART_STATE_TX );
}

#endif // __weak_symbol

/* Public Functions -------------------------------------------------------- */

void uartbuftx_Write(struct uartbuftx_s *t, const uint8_t *buf, size_t size)
{
	if ( size == 0 ) {
		uartbuftx_Flush(t);
		return;
	}

	if ( t->bufsize == 0 ) {
		// unbuffered write - synchronous mode
		uartbuftx_Flush(t);
		uartbuftx_Write_Callback(t, buf, size);
		uartbuftx_Flush(t);
	} else {
		// buffered write

		// are we able to get TxCpltInterrupt ?
		if ( t->use_interrupt ) {
			// TxCpltInterrupt may occur (if we are busy transmitting)
			bool flag = false;
			uartbuftx_ATOMIC_BLOCK(t) {
				// Will TxCpltInterrupt be called, cause we are transmitting, and
				// does t->buf fits into the free space in t->buf ?
				if ( _uartbuftx_IsWriting(t) && size < ( t->bufsize - t->nexttxpos ) ) {
					// copy buf into the free space in t->buf
					memcpy(&t->buf[t->nexttxpos], buf, size);
					// inform interrupt it will need to transmit more characters
					t->nexttxpos += size;
					// done here - return
					flag = true;
				}
			}
			// don't return from ATOMIC_BLOCK
			if ( flag ) return;
		}

		// does buf fits into t->buf ?
		if ( size > t->bufsize ) {
			// transmit all characters that don't fit in buffer in synchronous mode
			const size_t len = size - t->bufsize;
			uartbuftx_Flush(t);
			uartbuftx_Write_Callback(t, buf, len);
			size -= len;
			buf += len;
			// transmit rest of characters in asynchronous mode
		}

		uartbuftx_Flush(t);
		memcpy(t->buf, buf, size);
		t->nowtxpos = t->nexttxpos = size;
		uartbuftx_Write_Callback(t, t->buf, size);

		// continue transmission in interrupt
	}
}

void uartbuftx_Flush(struct uartbuftx_s *t)
{
	uartbuftx_Flush_Callback(t);
	assert( t->use_interrupt && !_uartbuftx_IsWriting(t) );
}

void uartbuftx_TxCplt_IRQHandler(struct uartbuftx_s *t)
{
	assert( t->nowtxpos <= t->nexttxpos );
	if ( t->nowtxpos < t->nexttxpos ) {
		const size_t size = t->nexttxpos - t->nowtxpos;
		memmove(&t->buf[0], &t->buf[t->nowtxpos], size);
		t->nowtxpos = t->nexttxpos = size;
		uartbuftx_Write_Callback(t, &t->buf[0], t->nowtxpos);
	} else {
		t->nowtxpos = t->nexttxpos = 0;
	}
}

