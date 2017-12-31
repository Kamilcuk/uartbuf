/*
 * uartbufrx.h
 *
 *  Created on: 08.11.2017
 *      Author: Kamil Cukrowski
 *     License: jointly under MIT License and Beerware License
 */
#ifndef UARTBUFRX_H_
#define UARTBUFRX_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // size_t
#include <sys/cdefs.h> // __nonnull

/* Exported Types ----------------------------------------------------- */

struct uartbufrx_s {
	uint8_t * const buf;
	const size_t bufsize;
	volatile size_t bufpos;
	size_t armpos;
#ifdef uartbufrx_priv_t_declared
	uartbufrx_priv_t priv;
#endif
};

/* Exported Macros ----------------------------------------------------- */

#define UARTBUFRX_INIT(_buf, _bufsize) \
	((struct uartbufrx_s){ \
		.buf = _buf, \
		.bufsize = _bufsize, \
	})

/* Callback Functions ----------------------------------------------- */
void uartbufrx_EnableIRQ_Callback(const struct uartbufrx_s *t) __nonnull((1));
void uartbufrx_DisableIRQ_Callback(const struct uartbufrx_s *t) __nonnull((1));
void uartbufrx_Receive_IT_Callback(const struct uartbufrx_s *t, uint8_t buf[], size_t size) __nonnull((1,2));
void uartbufrx_IsOverflowed_Callback(struct uartbufrx_s *t) __nonnull((1));

/* Exported Functions ---------------------------------------------------- */
const uint8_t *uartbufrx_buf(const struct uartbufrx_s *t) __nonnull((1));
uint8_t *uartbufrx_buf_nonconst(struct uartbufrx_s *t) __nonnull((1));
size_t uartbufrx_buflen(struct uartbufrx_s *t) __nonnull((1));
size_t uartbufrx_bufsize(const struct uartbufrx_s *t) __nonnull((1));
bool uartbufrx_IsOverflowed(struct uartbufrx_s *t) __nonnull((1));
void uartbufrx_Receive_IT(struct uartbufrx_s *t) __nonnull((1));
void uartbufrx_FlushN(struct uartbufrx_s *t, size_t n) __nonnull((1));
void uartbufrx_FlushN_NotOverflowed(struct uartbufrx_s *t, size_t n) __nonnull((1));
void uartbufrx_FlushAll(struct uartbufrx_s *t) __nonnull((1));
void uartbufrx_FlushAll_NotOverflowed(struct uartbufrx_s *t) __nonnull((1));

void uartbufrx_RxCplt_DMA_IRQHandler(struct uartbufrx_s *t, size_t size) __nonnull((1));
void uartbufrx_RxCplt_IRQHandler(struct uartbufrx_s *t, const uint8_t buf[], size_t size)  __nonnull((1,2));

#endif // UARTBUFRX_H_

