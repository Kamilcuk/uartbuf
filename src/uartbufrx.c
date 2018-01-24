/*
 * uartbuftx.c
 *
 *  Created on: 08.11.2017
 *      Author: Kamil Cukrowski
 *     License: jointly under MIT License and Beerware License
 */
#include "uartbufrx.h"

#include "_uartbuf.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

/* Private Macros -------------------------------------------- */

#define uartbufrx_ATOMIC_BLOCK(t) \
		for( bool _ToDo = (uartbufrx_DisableIRQ_Callback(t), true); \
		_ToDo; \
		_ToDo = (uartbufrx_EnableIRQ_Callback(t), false) )

/* Private Functions ---------------------------------------------------- */

static
size_t _uartbufrx_bufpos(struct uartbufrx_s *t)
{
	size_t tmp;
#if ! SIZE_T_ATOMIC_READ
	uartbufrx_ATOMIC_BLOCK(t)
#endif
	{
		tmp = t->pos;
	}
	return tmp;
}

static inline
bool _uartbufrx_IsOverflowed(struct uartbufrx_s *t)
{
	return uartbufrx_len(t) == t->size;
}

/* Exported Functions ------------------------------------------ */

inline
const uint8_t * uartbufrx_buf(const struct uartbufrx_s *t)
{
	return t->buf;
}

inline
uint8_t * uartbufrx_buf_nonconst(struct uartbufrx_s *t)
{
	return t->buf;
}

/**
 * Every time
 * @param t
 * @return
 */
inline
size_t uartbufrx_len(struct uartbufrx_s *t)
{
	uartbufrx_ArmReceive(t);
	return _uartbufrx_bufpos(t);
}

inline
size_t uartbufrx_size(const struct uartbufrx_s *t)
{
	return t->size;
}

/**
 * Get size of free space in buffer
 * @param t
 * @return
 */
inline
size_t uartbufrx_free(struct uartbufrx_s *t)
{
	return t->size - uartbufrx_len(t);
}

/**
 * Check for buffer overflow. If it occurs, callback is called
 * @param t
 * @return
 */
bool uartbufrx_IsOverflowed(struct uartbufrx_s *t)
{
	bool isOverflowed = _uartbufrx_IsOverflowed(t);
	if ( isOverflowed ) {
		uartbufrx_Overflowed_Callback(t);
	}
	return isOverflowed;
}

/**
 * Arm receiving data. This calls callback function if receiving is not armed
 * @param t
 */
void uartbufrx_ArmReceive(struct uartbufrx_s *t)
{
	if ( !uartbufrx_IsReceiving_Callback(t) ) {
		// we are not receiving any data,
		// so we do not need to fear that RxCplt_IRQHandlers will be called here
		// so we can access t->bufpos directly
		uartbufrx_ArmReceive_Callback(t,  &t->buf[t->pos], t->size - t->pos);
	}
}

/**
 * Flush number of data in buffer.
 * @param t
 * @param n
 */
void uartbufrx_FlushN(struct uartbufrx_s *t, size_t n)
{
//	UARTBUF_DBG("%zd %zd", n, uartbufrx_len(t));
	assert(n <= uartbufrx_len(t));
	if ( n == 0 ) return;
	/// now this is a little bit tricky
	/// we have the knownledge that RxCpltInt puts the data on the end of the buffer
	size_t out = 0;
	size_t in = n;
	bool synchronized = false;
	for(;;) {
		const volatile size_t buflensave = uartbufrx_len(t);
		assert( buflensave >= in );
		const size_t len = buflensave - in;
		// so we can copy from a saved end position to anywhere without ATOMIC_BLOCK
		memmove(&t->buf[out], &t->buf[in], len);
		out += len;

		// we should continue copying, until out position is synchronized with buffer positino in an ATOMIC_BLOCK
		// in another words, during the memmove aboce, no RxCpltInt occured to increment out t->bufpos
		uartbufrx_ATOMIC_BLOCK(t) {
			// we have memmoved all data from &t->buf[t->bufpos - n] to &t->buf[0]
			// however, t->bufpos may got incremented in RxCpltInt
			if ( buflensave == t->pos ) {
				// all ok, RxCpltInt did not occur during copying
				// we move t->bufpos to out, where out value should be equal to t->bufpos - n
				t->pos -= n;
				assert( out == t->pos );
				synchronized = true;
			}
		}
		// don't break from ATOMIC_BLOCK
		if ( synchronized ) {
			break;
		}

		in = buflensave;
	}
}

/**
 * Flush all data buffer
 * @param t
 */
inline
void uartbufrx_FlushAll(struct uartbufrx_s *t)
{
#if ! SIZE_T_ATOMIC_READ
	uartbufrx_ATOMIC_BLOCK(t)
#endif
	{
		t->pos = 0;
	}
}

void uartbufrx_RxCplt_DMA_IRQHandler(struct uartbufrx_s *t, uint8_t buf[], size_t size)
{
	// buf must be a pointer in the free space of our buffer
	assert( &t->buf[t->pos] <= buf && buf <= &t->buf[t->size] );
	// size must be lower then free space in out buffer
	assert( size <= t->size - t->pos );

	// if bufpos was moved, DMA copied data to a wrong region
	if ( __predict_false(&t->buf[t->pos] != buf) ) {
		// move it to bufpos
		memmove(&t->buf[t->pos], buf, size);
	}

	// increment bufpos to notify that new data were received
	t->pos += size;
	// re-arm receiving
	uartbufrx_ArmReceive_IRQHandler_Callback(t, &t->buf[t->pos], t->size - t->pos);
}

/**
 * uartbuf register Rx completed interrupt
 * Tells uartbuf library, that buf was received
 * @param t
 * @param buf
 * @param size
 */
void uartbufrx_RxCplt_IRQHandler(struct uartbufrx_s *t, const uint8_t buf[restrict], size_t size)
{
	// buf is restricted - must be outside out buffer
	assert( buf < &t->buf[0] || &t->buf[t->size] < buf );

	// size cannot be greater then free size in out buffer
	const size_t freesize = t->size - t->pos;
	if ( size > freesize ) {
		size = freesize;
	}

	// copy input data into our buffer
	memcpy(&t->buf[t->pos], buf, size);

	// increment bufpos to notify that new data were received
	t->pos += size;
	// re-arm receiving
	uartbufrx_ArmReceive_IRQHandler_Callback(t, &t->buf[t->pos], t->size - t->pos);
}


/* Exported Callback Functions ------------------------------------------ */

/**
 * Enable RxCplt_IRQHandler to be called. Leave critical section.
 * @param t
 */
__weak_symbol
void uartbufrx_EnableIRQ_Callback(const struct uartbufrx_s *t)
{
#if UARTBUFRX_USE_PNT_CALLBACKS
	if ( t->EnableIRQ ) t->EnableIRQ(t);
#endif // UARTBUFRX_USE_PNT_CALLBACKS
	// NVIC_Enable_IRQ(IRQ_UART2);
}

/**
 * Dissallow RxCplt_IRQHandler to be called. Enter critical section.
 * @param t
 */
__weak_symbol
void uartbufrx_DisableIRQ_Callback(const struct uartbufrx_s *t)
{
#if UARTBUFRX_USE_PNT_CALLBACKS
	if ( t->DisableIRQ ) t->DisableIRQ(t);
#endif // UARTBUFRX_USE_PNT_CALLBACKS
	// NVIC_Disable_IRQ(IRQ_UART2);
}

__weak_symbol
bool uartbufrx_IsReceiving_Callback(const struct uartbufrx_s *t)
{
#if UARTBUFRX_USE_PNT_CALLBACKS
	if ( t->IsReceiving ) return t->IsReceiving(t);
#endif // UARTBUFRX_USE_PNT_CALLBACKS
	// return t->priv.huart.gstate != HAL_UART_STATE_RX;
	return false;
}

/**
 * Arm receiving data. This gets called from application.
 * This function should:
 * - check if RxCplt_IRQHandler is armed
 * - if it is, this function should return, and buf and size arguments may be invalid
 *   (as RxCplt_IRQHandler may be called prior to calling this function)
 * - if it isn't, this function should arm receiving data to receive to buf with size (at maximum)
 * - if data are to be received directly to buf (like in DMA transfer...),
 *   call RxCplt_DMA_IRQHandler to notify uartbufrx of incoming data
 * - if data were received in some external buffer, call RxCplt_IRQHandler to notify uartbufrx
 * @param t
 * @param buf pointer to free buffer ready to receive data
 * @param size size of buf
 */
__weak_symbol
void uartbufrx_ArmReceive_Callback(struct uartbufrx_s *t, uint8_t buf[], size_t size)
{
#if UARTBUFRX_USE_PNT_CALLBACKS
	if ( t->ArmReceive ) t->ArmReceive(t, buf, size);
#endif // UARTBUFRX_USE_PNT_CALLBACKS
	// HAL_UART_Receive_DMA(t->priv.huart, buf, size, 10);
	// or
	// HAL_UART_Receive(t->priv.huart, buf, size, 1000);
	// uartbufrx_RxCplt_DMA_IRQHandler(t, size);
	// or
	// uint8_t mybuffer[10];
	// HAL_UART_Receive(t->priv.huart, mybuffer, 10, 1000);
	// uartbufrx_RxCplt_IRQHandler(t, mybuffer, 10);
}

/**
 * This is the same as uartbufrx_ArmReceive_Callback only called from an _IRQHandler
 * @param t
 * @param buf
 * @param size
 */
__weak_symbol
void uartbufrx_ArmReceive_IRQHandler_Callback(struct uartbufrx_s *t, uint8_t buf[], size_t size)
{
#if UARTBUFRX_USE_PNT_CALLBACKS
	if ( t->ArmReceive_IRQHandler ) t->ArmReceive_IRQHandler(t, buf, size);
#else
	uartbufrx_ArmReceive_Callback(t, buf, size);
#endif // UARTBUFRX_USE_PNT_CALLBACKS
	// HAL_UART_Receive_DMA(t->priv.huart, buf, size, 10);
}

/**
 * Callback for notification of buffer overflow
 * @param t
 */
__weak_symbol
void uartbufrx_Overflowed_Callback(struct uartbufrx_s *t)
{
#if UARTBUFRX_USE_PNT_CALLBACKS
	if ( t->Overflowed ) {
		t->Overflowed(t);
	} else
#endif
	{
		fprintf(stderr, "uartbufrx overflowed %zu/%zu!\n", uartbufrx_len(t), uartbufrx_size(t));
	}
}
