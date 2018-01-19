/*
 * uartbuf_board.h
 *
 *  Created on: 14 sty 2018
 *      Author: kamil
 */

#ifndef TEST_UARTBUF_BOARD_H_
#define TEST_UARTBUF_BOARD_H_

#include <stdio.h>
#include <stdint.h>

#define UARTBUF_DBG(str, ...)  printf("%s:%d: " str "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define UARTBUFRX_USE_PNT_CALLBACKS 1

#define UARTBUFTX_USE_PNT_CALLBACKS 1

#define uartbufrx_priv_t_declared 1

#define uartbuftx_priv_t_declared 1

struct uartbuf_priv_s {
	int internal;
	void *arg;

	struct {
		uint8_t *buf;
		size_t size;
	} test2_data;
};

typedef struct uartbuf_priv_s  uartbufrx_priv_t;

typedef struct uartbuf_priv_s  uartbuftx_priv_t;

#endif /* TEST_UARTBUF_BOARD_H_ */
