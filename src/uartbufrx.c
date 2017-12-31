/*
 * uartbuftx.c
 *
 *  Created on: 08.11.2017
 *      Author: Kamil Cukrowski
 *     License: jointly under MIT License and Beerware License
 */
#include "_uartbuf.h"
#include "uartbufrx.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Private Macros -------------------------------------------- */

#define uartbufrx_ATOMIC_BLOCK(t) \
		for( bool _ToDo = (uartbufrx_DisableIRQ_Callback(t), true); \
		_ToDo; \
		_ToDo = (uartbufrx_EnableIRQ_Callback(t), false) )

#define PRINTERR(...)   fprintf(stderr, __VA_ARGS__)
//#define PRINTERR(...) do { __asm__ __volatile__ ("bkpt #0"); printf(__VA_ARGS__); }while(0)
#define uartbuf_DEBUG 0
#if defined(uartbuf_DEBUG) && uartbuf_DEBUG
#define PRINTDBG(...)   printf(__VA_ARGS__)
#define PRINTDBGARR(how, arr, size, stop, start, ...) \
		do { \
			printf("%s:" start, __FUNCTION__, ##__VA_ARGS__); \
			for(int _i = 0; _i < (size); ++_i) { \
				printf( (how), (arr)[_i] ); \
			} \
			printf(stop); \
		} while(0)
#else
#define PRINTDBG(...)
#define PRINTDBGARR(...)
#endif

/* Private Functions ---------------------------------------------------- */

static inline
size_t _uartbufrx_bufpos(struct uartbufrx_s *t)
{
	size_t tmp;
#if SIZE_T_ATOMIC_READ
	uartbufrx_ATOMIC_BLOCK(t)
#endif
	{
		tmp = t->bufpos;
	}
	return tmp;
}

static
void _uartbufrx_Receive_IT(struct uartbufrx_s *t)
{
	size_t bufpos = _uartbufrx_bufpos(t);
	const size_t freesize = t->bufsize - bufpos;
	if ( freesize ) {
		uint8_t * const buf = &t->buf[bufpos];
		uartbufrx_Receive_IT_Callback(t, buf, freesize);
	}
}

static inline
bool _uartbufrx_IsOverflowed(struct uartbufrx_s *t)
{
	return uartbufrx_buflen(t) == t->bufsize;
}


/* Exported Callback Functions ------------------------------------------ */

#ifdef __weak_symbol

__weak_symbol
void uartbufrx_EnableIRQ_Callback(const struct uartbufrx_s *t)
{
}

__weak_symbol
void uartbufrx_DisableIRQ_Callback(const struct uartbufrx_s *t)
{
}

__weak_symbol
void uartbufrx_Receive_IT_Callback(const struct uartbufrx_s *t, uint8_t buf[], size_t size)
{
	// check if RxCplt_IRQHandler is armed
	// if it is, then buf and size args are invalid and this function should return
	// it it isn't, buf points to receive buffer of size length
	// this function should arm receiving size data to buf
	// HAL_UART_Receive_DMA(t->priv.huart, buf, size, 10);
}

__weak_symbol
void uartbufrx_IsOverflowed_Callback(struct uartbufrx_s *t)
{
	PRINTERR("uartbufrx overflowed!\n");
	uartbufrx_FlushAll(t);
}

#endif // __weak_symbol

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

inline
size_t uartbufrx_buflen(struct uartbufrx_s *t)
{
	/// _uartbufrx_bufpos is called twice
	/// once in Receive_IT and once when returning
	/// this is, because Receive_IT may be used for synchronous receiving
	_uartbufrx_Receive_IT(t);
	return _uartbufrx_bufpos(t);
}

inline
size_t uartbufrx_bufsize(const struct uartbufrx_s *t)
{
	return t->bufsize;
}

bool uartbufrx_IsOverflowed(struct uartbufrx_s *t)
{
	bool isOverflowed = _uartbufrx_IsOverflowed(t);
	if ( isOverflowed ) {
		uartbufrx_IsOverflowed_Callback(t);
	}
	return isOverflowed;
}

void uartbufrx_Receive_IT(struct uartbufrx_s *t)
{
	_uartbufrx_Receive_IT(t);
}

inline
void uartbufrx_FlushAll(struct uartbufrx_s *t)
{
	while(uartbufrx_IsOverflowed(t));
	uartbufrx_FlushAll_NotOverflowed(t);
}

void uartbufrx_FlushAll_NotOverflowed(struct uartbufrx_s *t)
{
	uartbufrx_ATOMIC_BLOCK(t) {
		t->bufpos = 0;
	}
}

inline
void uartbufrx_FlushN(struct uartbufrx_s *t, size_t n)
{
	while(uartbufrx_IsOverflowed(t));
	return uartbufrx_FlushN_NotOverflowed(t, n);
}

void uartbufrx_FlushN_NotOverflowed(struct uartbufrx_s *t, size_t n)
{
	UARTBUF_DBG("%zd %zd", n, uartbufrx_buflen(t));
	assert(n <= uartbufrx_buflen(t));
	if ( n == 0 ) return;
	/// now this is a little bit tricky
	/// we have the knownledge that RxCpltInt puts the data on the end of the buffer
	size_t out = 0;
	size_t in = n;
	bool synchronized = false;
	for(;;) {
		const size_t possave = uartbufrx_buflen(t);
		assert( possave >= in );
		const size_t len = possave - in;
		// so we can copy from a saved end position to anywhere without ATOMIC_BLOCK
		memmove(&t->buf[out], &t->buf[in], len);
		out += len;
		// we should continue copying, until out position is synchronized with buffer positino in an ATOMIC_BLOCK
		// in another words, during the memmove aboce, no RxCpltInt occured to increment out t->bufpos
		uartbufrx_ATOMIC_BLOCK(t) {
			// we have memmoved all data from &t->buf[t->bufpos - n] to &t->buf[0]
			// however, t->bufpos may got incremented in RxCpltInt
			if ( possave == t->bufpos ) {
				// all ok, RxCpltInt did not occur during copying
				// we move t->bufpos to out, where out value should be equal to t->bufpos - n
				assert( out == t->bufpos - n );
				t->bufpos = out;
				synchronized = true;
			}
		}
		if ( synchronized ) {
			break;
		}
		in = possave;
	}
}

void uartbufrx_RxCplt_DMA_IRQHandler(struct uartbufrx_s *t, size_t size)
{
	assert(t->bufpos <= t->armpos);
	const size_t buffree = t->bufsize - t->armpos;
	assert( size <= buffree );
	// if bufpos was moved, DMA copied data to a wrong region
	if ( __predict_false(t->bufpos != t->armpos) ) {
		// move it to bufpos
		memmove(&t->buf[t->bufpos], &t->buf[t->armpos], size);
	}
	// increment bufpos to notify that new data were received
	t->armpos = t->bufpos += size;
	// re-arm receiving
	uartbufrx_Receive_IT(t);
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
	const size_t maxsize = t->bufsize - t->bufpos;
	if ( __predict_false(size > maxsize) ) {
		size = maxsize;
	}
	// copy input data into buffer
	memcpy(&t->buf[t->bufpos], &buf[0], size);
	// increment bufpos to notify that new data were received
	t->armpos = t->bufpos += size;
	// re-arm receiving
	uartbufrx_Receive_IT(t);
}

