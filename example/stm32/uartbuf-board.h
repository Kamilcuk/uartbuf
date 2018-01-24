/*
 * uartbuf_board.h
 *
 *  Created on: 23.01.2018
 *      Author: kamil
 */

#ifndef SRC_SYSTEM_UARTBUF_BOARD_H_
#define SRC_SYSTEM_UARTBUF_BOARD_H_


#include "hal.h"

typedef struct {
	UART_HandleTypeDef *huart;
	IRQn_Type IRQn;

	size_t size;

} uartbuftx_priv_t;
#define uartbuftx_priv_t_declared 1

typedef struct {
	UART_HandleTypeDef *huart;
	IRQn_Type IRQn;
	size_t maxchunk;
} uartbufrx_priv_t;
#define uartbufrx_priv_t_declared 1

#endif /* SRC_SYSTEM_UARTBUF_BOARD_H_ */
