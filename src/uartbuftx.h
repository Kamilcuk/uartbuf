/*
 * uartbuftx.h
 *
 *  Created on: 08.11.2017
 *      Author: Kamil Cukrowski
 *     License: jointly under MIT License and Beerware License
 */
#ifndef UARTBUFTX_H_
#define UARTBUFTX_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // size_t

/* Exported Types ------------------------------------------------- */

struct uartbuftx_s {
	bool use_interrupt;
	uint8_t * const buf;
	volatile size_t nowtxpos;
	volatile size_t nexttxpos;
	const size_t bufsize;
#ifdef uartbuftx_priv_t_declared
	uartbuftx_priv_t priv;
#endif
};

/* Exported Macros -------------------------------------------------- */

#define UARTBUFTX_INIT( _buf, _bufsize, _use_interrupt) ((struct uartbuftx_s){ \
	.buf = _buf, \
	.bufsize = _bufsize, \
	.use_interrupt = _use_interrupt, \
	})


/* Callback Functions -------------------------------------------- */
void uartbuftx_EnableIRQ_Callback(const struct uartbuftx_s *t);
void uartbuftx_DisableIRQ_Callback(const struct uartbuftx_s *t);
void uartbuftx_Write_Callback(const struct uartbuftx_s *t, const uint8_t buf[], size_t size);
void uartbuftx_Flush_Callback(const struct uartbuftx_s *t);

/* Exported Functions ----------------------------------------------- */
void uartbuftx_Write(struct uartbuftx_s *t, const uint8_t *buf, size_t size);
void uartbuftx_Flush(struct uartbuftx_s *t);
void uartbuftx_TxCplt_IRQHandler(struct uartbuftx_s *t);

#endif // UARTBUFTX_H_

