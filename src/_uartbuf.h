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

#include <errno.h>
#include <stdint.h>
#ifdef __unix
#include <sys/cdefs.h> // __predict_false __weak_symbol
#include <bits/types.h>
#endif

#ifndef ETIMEDOUT
#define ETIMEDOUT 4039
#endif // ETIMEDOUT

#ifndef ENOBUFS
#define ENOBUFS 4060
#endif // ENOBUFS

#ifndef __weak_symbol
# ifdef __weak
#  define __weak_symbol  __weak
# else // __weak
#  ifdef __GNUC__
#   define __weak_symbol  __attribute__((__weak__))
#  endif // __GNUC__
# endif // __weak
#endif // __weak_symbol

#ifndef __predict_false
# define __predict_false(expr)  (expr)
#endif // __predict_false

// SIZE_T_ATOMIC_READ
// This macro should be set to 1,
// if type of size_t can be read by a single cpu cycle
#ifndef SIZE_T_ATOMIC_READ
# ifndef SIZE_MAX
#  warning Your Compiler is broken and not C99 compliant. SIZE_MAX should be defined in limits.h
#  define SIZE_MAX  UINT_MAX
# endif // SIZE_MAX
# ifndef WORD_MAX
#  ifdef __WORDSIZE
#   define WORD_MAX  ((1UL<<__WORDSIZE)-1UL)
#  else // __WORDSIZE
#   define WORD_MAX  UINT_MAX
#  endif // __WORDSIZE
# endif // WORD_MAX
# if SIZE_MAX <= WORD_MAX
#  define SIZE_T_ATOMIC_READ 1
# endif
#endif // SIZE_T_ATOMIC_READ

#define UARTBUF_DBG(str, ...)  printf("%s:%d: " str "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#ifndef UARTBUF_DBG
# define UARTBUF_DBG(str, ...)
#endif

#endif // _UARTBUF_H_

