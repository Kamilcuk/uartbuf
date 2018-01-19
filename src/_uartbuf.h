/*
 * _uartbuf.h
 *
 *  Created on: 08.11.2017
 *      Author: Kamil Cukrowski
 *     License: jointly under MIT License and Beerware License
 */
/*
 * Private uartbuf definitions
 */
#ifndef _UARTBUF_H_
#define _UARTBUF_H_

#include <uartbuf_board.h>

#include <errno.h>
#include <stdint.h>
#include <limits.h>
#include <assert.h>
#include <sys/cdefs.h> // __predict_false __weak_symbol
#include <bits/types.h>

#ifndef ETIMEDOUT
#define ETIMEDOUT 4039
#endif // ETIMEDOUT

#ifndef ENOBUFS
#define ENOBUFS 4060
#endif // ENOBUFS

#ifndef __weak_symbol
# ifdef __GNUC__
#  define __weak_symbol  __attribute__((__weak__))
# endif // __GNUC__
#endif // __weak_symbol

#ifndef __predict_false
# define __predict_false(expr)  (expr)
#endif // __predict_false

// SIZE_T_ATOMIC_READ
// This macro should be set to 1,
// if type of size_t can be read by a single cpu cycle
#ifndef SIZE_T_ATOMIC_READ
# if SIZE_MAX <= ULONG_MAX
#  define SIZE_T_ATOMIC_READ 0
# endif
#endif // SIZE_T_ATOMIC_READ

//#define UARTBUF_DBG(str, ...)  printf("%s:%d: " str "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#ifndef UARTBUF_DBG
# define UARTBUF_DBG(str, ...)
#endif

#define unitassert(expr) assert(expr)
#ifndef unitassert
# define unitassert(expr) /* */
#endif

#endif // _UARTBUF_H_

