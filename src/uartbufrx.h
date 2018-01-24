/*
 * uartbufrx.h
 *
 *  Created on: 08.11.2017
 *      Author: Kamil Cukrowski
 *     License: jointly under MIT License and Beerware License
 */
#ifndef UARTBUFRX_H_
#define UARTBUFRX_H_

#include <uartbuf-board.h> // uartbufrx_priv_t_declared UARTBUFRX_USE_PNT_CALLBACKS

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/cdefs.h> // __nonnull

/* Exported Types ----------------------------------------------------- */

struct uartbufrx_s {

	// pointer to memory location
	uint8_t * const buf;
	// buf size
	const size_t size;
	// position in buffer
	volatile size_t pos;

#if UARTBUFRX_USE_PNT_CALLBACKS
	void (*EnableIRQ)(const struct uartbufrx_s *t);
	void (*DisableIRQ)(const struct uartbufrx_s *t);
	bool (*IsReceiving)(const struct uartbufrx_s *t);
	void (*ArmReceive)(struct uartbufrx_s *t, uint8_t buf[], size_t size);
	void (*ArmReceive_IRQHandler)(struct uartbufrx_s *t, uint8_t buf[], size_t size);
	void (*Overflowed)(const struct uartbufrx_s *t);
#endif // UARTBUFRX_USE_CALLBACKS

#ifdef uartbufrx_priv_t_declared
	uartbufrx_priv_t priv;
#endif // uartbufrx_priv_t_declared
};

/* Exported Macros ----------------------------------------------------- */

#define UARTBUFRX_INIT(_buf, _bufsize) \
	((struct uartbufrx_s){ \
		.buf = _buf, \
		.bufsize = _bufsize, \
	})

/* Exported Functions ---------------------------------------------------- */

const uint8_t *uartbufrx_buf(const struct uartbufrx_s *t)
	__attribute_pure__ __nonnull_all;
uint8_t *uartbufrx_buf_nonconst(struct uartbufrx_s *t)
	__attribute_pure__ __nonnull_all;
size_t uartbufrx_len(struct uartbufrx_s *t)
	__nonnull_all;
size_t uartbufrx_size(const struct uartbufrx_s *t)
	__attribute_pure__ __nonnull_all;
size_t uartbufrx_free(struct uartbufrx_s *t)
	__nonnull_all;
bool uartbufrx_IsOverflowed(struct uartbufrx_s *t)
	__nonnull_all;

void uartbufrx_ArmReceive(struct uartbufrx_s *t)
	__nonnull_all;
void uartbufrx_FlushN(struct uartbufrx_s *t, size_t n)
	__nonnull_all;
void uartbufrx_FlushAll(struct uartbufrx_s *t)
	__nonnull_all;

void uartbufrx_RxCplt_DMA_IRQHandler(struct uartbufrx_s *t, uint8_t buf[], size_t size)
	__nonnull_all;
void uartbufrx_RxCplt_IRQHandler(struct uartbufrx_s *t, const uint8_t buf[], size_t size)
	__nonnull_all;


/* Callback Functions ----------------------------------------------- */
void uartbufrx_EnableIRQ_Callback(const struct uartbufrx_s *t)
	__nonnull_all;
void uartbufrx_DisableIRQ_Callback(const struct uartbufrx_s *t)
	__nonnull_all;
bool uartbufrx_IsReceiving_Callback(const struct uartbufrx_s *t)
	__nonnull_all;
void uartbufrx_ArmReceive_Callback(struct uartbufrx_s *t, uint8_t buf[], size_t size)
	__nonnull_all;
void uartbufrx_ArmReceive_IRQHandler_Callback(struct uartbufrx_s *t, uint8_t buf[], size_t size)
	__nonnull_all;
void uartbufrx_Overflowed_Callback(struct uartbufrx_s *t)
	__nonnull_all;

#endif // UARTBUFRX_H_

