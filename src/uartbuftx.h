/*
 * uartbuftx.h
 *
 *  Created on: 08.11.2017
 *      Author: Kamil Cukrowski
 *     License: jointly under MIT License and Beerware License
 */
#ifndef UARTBUFTX_H_
#define UARTBUFTX_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/cdefs.h> // __nonnull
#include <uartbuf-board.h> // UARTBUFTX_USE_PNT_CALLBACKS uartbuftx_priv_t_declared

/* Exported Types ------------------------------------------------- */

struct uartbuftx_s {

	// allocated buffer space
	uint8_t * const buf;
	// buf size
	const size_t size;
	// head of fifo
	volatile size_t head;
	// tail of fifo
	volatile size_t tail;

#if UARTBUFTX_USE_PNT_CALLBACKS
	void (*EnableIRQ)(const struct uartbuftx_s *t);
	void (*DisableIRQ)(const struct uartbuftx_s *t);
	void (*Write)(struct uartbuftx_s *t, const uint8_t buf[], size_t size);
	void (*Write_IRQHandler)(struct uartbuftx_s *t, const uint8_t buf[], size_t size);
	void (*Flush)(const struct uartbuftx_s *t);
#endif // UARTBUFTX_USE_PNT_CALLBACKS

	// private data
#ifdef uartbuftx_priv_t_declared
	uartbuftx_priv_t priv;
#endif

};

/* Exported Macros -------------------------------------------------- */

#define UARTBUFTX_INIT( _buf, _bufsize) \
	((struct uartbuftx_s){ \
		.buf = _buf, \
		.bufsize = _bufsize, \
	})

/* Exported Functions ----------------------------------------------- */

bool uartbuftx_IsWriting(const struct uartbuftx_s *t)
	__attribute_pure__ __nonnull_all;
void uartbuftx_Write(struct uartbuftx_s *t, const uint8_t buf[restrict], size_t size)
	__nonnull_all;
void uartbuftx_Flush(struct uartbuftx_s *t)
	__nonnull_all;
void uartbuftx_printf(struct uartbuftx_s *t)
	__nonnull_all;

void uartbuftx_TxCplt_IRQHandler(struct uartbuftx_s *t, size_t size)
	__nonnull_all;

/* Callback Functions -------------------------------------------- */
void uartbuftx_EnableIRQ_Callback(const struct uartbuftx_s *t)
	__nonnull_all;
void uartbuftx_DisableIRQ_Callback(const struct uartbuftx_s *t)
	__nonnull_all;
bool uartbuftx_IsWriting_Callback(const struct uartbuftx_s *t)
	__nonnull_all;
void uartbuftx_Write_Callback(struct uartbuftx_s *t, const uint8_t buf[], size_t size)
	__nonnull_all;
void uartbuftx_Write_IRQHandler_Callback(struct uartbuftx_s *t, const uint8_t buf[], size_t size)
	__nonnull_all;
void uartbuftx_Flush_Callback(const struct uartbuftx_s *t)
	__nonnull_all;

#endif // UARTBUFTX_H_

