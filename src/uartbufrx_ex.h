/*
 * uartbuf.h
 *
 *  Created on: 08.11.2017
 *      Author: Kamil Cukrowski
 *     License: jointly under MIT License and Beerware License
 */
#ifndef UARTBUFRX_EX_H_
#define UARTBUFRX_EX_H_

#include "uartbufrx.h"
#include "timeout.h" // timeout_t

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/cdefs.h> // __nonnull

/* uartbuf rx ------------------------------------------------------- */

size_t uartbufrx_waitfor_buflen_f(struct uartbufrx_s *t, size_t num, bool stop(void *), void *arg);
size_t uartbufrx_waitfor_buflen_t(struct uartbufrx_s *t, size_t num, timeout_t *timeout);
size_t uartbufrx_waitfor_buflen(struct uartbufrx_s *t, size_t num, unsigned int timeout);

size_t uartbufrx_Read_t(struct uartbufrx_s *t, uint8_t buf[], size_t size, timeout_t *timeout);
size_t uartbufrx_Read(struct uartbufrx_s *t, uint8_t buf[], size_t size, unsigned int timeout);

/* uartbufrx findmsg ---------------------------------------------------------------------------------------- */

struct uartbufrx_findmsgconf_s {
	/**
	 * Minimum message length
	 */
	size_t minMsgLen;

	/**
	 * Maximum message length
	 * if maxMsgLen == 0, then (uartbufrx_s).bufsize value is used.
	 */
	size_t maxMsgLen;

	/**
	 * Checks if message starting at buf which is at least minMsgLen long is a message beginning
	 * @param buf
	 * @param size buf length, equal to minMsgLen
	 * @param arg private argument
	 * @return < 0 on error
	 *         = 0 when buf is not the beginning of the message
	 *         > 0 the assumed minimal message length if buf is the beginning of the message
	 */
	int (*checkBeginning)(const uint8_t buf[], size_t minMsgLen, void *arg);

	/**
	 * Check if message starting at buf and size long is a valid message;
	 * This will be called for every size from [minMsgLen, maxMsgLen] until valid message is received.
	 * Function should firstly check for message ending, and then check for validity (ie. CRC).
	 * @param buf
	 * @param size buf length, between maxMsgLen and minMsgLen
	 * @param arg private argument
	 * @return < 0 on error, invalid message, ex. message crc errors etc.
	 *         = 0 size is too small, waiting for more characters,
	 *         > 0 buf contains a valid message of size characters
	 */
	int (*checkEnding)(const uint8_t buf[], size_t size, void *arg);
};

int uartbufrx_findmsg_beginning(struct uartbufrx_s *t, size_t minlen,
		int (*checkBeginning)(const uint8_t buf[], size_t minlen, void *arg), void *arg, timeout_t *timeout)
		__nonnull_all;
int uartbufrx_findmsg_ending(struct uartbufrx_s *t, size_t startlen, size_t maxlen,
		int (*checkEnding)(const uint8_t buf[], size_t len, void *arg), void *arg, timeout_t *timeout)
		__nonnull_all;

int uartbufrx_findmsg(struct uartbufrx_s *t, const uint8_t **msg,
		const struct uartbufrx_findmsgconf_s *conf, void *arg, unsigned int Timeout);
void uartbufrx_findmsg_next(struct uartbufrx_s *t, size_t size)
		__nonnull_all;

int uartbufrx_getmsg(struct uartbufrx_s *t, const struct uartbufrx_findmsgconf_s * restrict conf,
		uint8_t buf[restrict], size_t buflen, unsigned int timeout)
		__nonnull_all;
bool uartbufrx_waitformsg(struct uartbufrx_s * restrict t,
		const uint8_t msg[restrict], size_t msgsize, unsigned int timeout)
		__nonnull_all;

void uartbufrx_waitfor_buflen_Callback(const struct uartbufrx_s *t, size_t num);

#endif /* UARTBUFRX_EX_H_ */

