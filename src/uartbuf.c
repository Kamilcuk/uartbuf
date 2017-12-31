/*
 * uartbuf.c
 *
 *  Created on: 08.11.2017
 *      Author: Kamil Cukrowski
 *     License: jointly under MIT License and Beerware License
 */
#include "_uartbuf.h"
#include "uartbuf.h"
#include "timeout.h" // timeout_t

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

/* Private Functions ------------------------------------------------------- */

/* Exported Functions -------------------------------------------------------- */

size_t uartbufrx_buflen_waitfor_f(struct uartbufrx_s *t, size_t num, bool stop(void *), void *arg)
{
	size_t ret;
	while( (ret = uartbufrx_buflen(t)) < num && !stop(arg) );
	return ret;
}

size_t uartbufrx_buflen_waitfor_t(struct uartbufrx_s *t, size_t num, timeout_t *Timeout)
{
	UARTBUF_DBG("%zd %u", num, timeout_get(Timeout));
	size_t ret;
	while( (ret = uartbufrx_buflen(t)) < num && !timeout_expired(Timeout) );
	return ret;
}

inline
size_t uartbufrx_buflen_waitfor(struct uartbufrx_s *t, size_t num, uint32_t Timeout)
{
	timeout_t to = timeout_create(Timeout);
	return uartbufrx_buflen_waitfor_t(t, num, &to);
}

size_t uartbufrx_Read_t(struct uartbufrx_s *t, uint8_t buf[], size_t size, timeout_t *Timeout)
{
	const size_t size0 = size;
	for(size_t buflen;
		size && (buflen = uartbufrx_buflen_waitfor_t(t, size, Timeout)) > 0;
		uartbufrx_FlushN(t, buflen)) {
		memcpy(buf, uartbufrx_buf(t), buflen);
		buf += buflen;
		size -= buflen;
	}
	return size0 - size;
}

inline
size_t uartbufrx_Read(struct uartbufrx_s *t, uint8_t buf[], size_t size, uint32_t Timeout)
{
	timeout_t htimeout = timeout_create(Timeout);
	return uartbufrx_Read_t(t, buf, size, &htimeout);
}

/* Upper Level Functions ---------------------------------------------------- */

/**
 * Calls checkBeggining for every position in receive buffer if it is at least minlen long in specified timeout.
 * @param t
 * @param minlen
 * @param checkBeginning  same function as described in (uartbufrx_findmsgconf_s)
 * @param arg
 * @param Timeout
 * @return checkBeginning returned value
 *         -ETIMEDOUT - on timeout
 */
int uartbufrx_findmsg_beginning(struct uartbufrx_s *t, size_t minlen,
								int (*checkBeginning)(const uint8_t *buf, size_t minlen, void *arg), void *arg,
								uint32_t Timeout)
{
	assert(minlen && minlen < uartbufrx_bufsize(t));
	assert(checkBeginning);
	timeout_t ctimeout = timeout_create(Timeout);
	int ret;
	for(size_t buflen;
		(buflen = uartbufrx_buflen_waitfor_t(t, minlen, &ctimeout)) >= minlen ;
		uartbufrx_FlushN(t, buflen - 1) ) {
		// now we have at least minlen characters in buffer
		// for every minlen consecutive characters in buffer call checkBeggining function
		const size_t idxmax = buflen - minlen; // max index to leave at least minlen characters
		const uint8_t * const rxbuf = uartbufrx_buf(t);
		for(size_t i = 0; i <= idxmax; ++i) {
			// for every position at least minlen long, check for message beginning
			if ( (ret = checkBeginning(&rxbuf[i], minlen, arg)) != 0 ) {
				if ( ret > 0 ) {
					// beginning found! flush i characters
					uartbufrx_FlushN(t, i);
				}
				// error or not - return ret
				return ret;
			}
		}
		// no beginning found in any minlen strings in buf
	}
	return -ETIMEDOUT;
}

/**
 * Wait for more characters in buffer up until maxlen until checkending returns != 0 in specified timeout
 * @param t
 * @param startlen
 * @param maxlen maximum length of chars in buffer
 * @param checkEnding same function as described in (uartbufrx_findmsgconf_s)->checkEnding
 * @param arg
 * @param Timeout
 * @return checkEnding returned value
 *         -ENOBUFS - no more space in buffer
 *         -ENOTIMEOUT - on timeout
 */
int uartbufrx_findmsg_ending(struct uartbufrx_s *t, size_t startlen, size_t maxlen,
							 int (*checkEnding)(const uint8_t *buf, size_t len, void *arg), void *arg,
							 uint32_t Timeout)
{
	assert(maxlen <= uartbufrx_bufsize(t));
	assert(checkEnding);
	if ( maxlen == 0 ) {
		maxlen = uartbufrx_bufsize(t);
	}
	timeout_t ctimeout = timeout_create(Timeout);
	for(size_t len = startlen, buflen;
		(buflen = uartbufrx_buflen_waitfor_t(t, len, &ctimeout)) >= len;
		++len) {
		for(; len <= buflen; ++len) {

			int ret;
			if ( (ret = checkEnding(uartbufrx_buf(t), len, arg)) != 0 ) {
				if ( ret < 0 ) {
					return ret;
				}
				return len;
			}

			if ( len >= maxlen ) {
				// we allow up to maxlen in our buffer
				// PRINTERR("ENOBUFS %zu %zu \n", startlen, maxlen);
				return -ENOBUFS;
			}
		}
	}
	return -ETIMEDOUT;
}

/**
 * Find message in receive buffer
 * @param t
 * @param msg filled with pointer to message beginning
 * @param conf specifies functions for message finding
 * @param arg argument passed to conf-> functions
 * @param Timeout
 * @return < 0 on error, otherwise message length
 */
int uartbufrx_findmsg(struct uartbufrx_s *t,
						  const uint8_t **msg,
						  const struct uartbufrx_findmsgconf_s *conf,
					  void *arg, uint32_t Timeout)
{
	timeout_t ctimeout = timeout_create(Timeout);
	for( ; ; uartbufrx_FlushN(t, 1) ) {
		int ret;

		ret = uartbufrx_findmsg_beginning(t,
										  conf->minMsgLen,
										  conf->checkBeginning, arg,
										  timeout_get(&ctimeout));
		if ( ret < 0 ) {
			// timeout!
			return ret;
		}
		// message beggining found

		// expectedMsgLen(ret) is longer then our buffer can hold
		if ( ret > uartbufrx_bufsize(t) ) {
			// try again, but this is alarming and should not happen
			// fprintf(stderrr, "expected %zu rxbufsize %zu\n", expectedMsgLen, uartbufrx_bufsize(t));
			continue;
		}

		ret = uartbufrx_findmsg_ending(t,
									   ret, conf->maxMsgLen,
									   conf->checkEnding, arg,
									   timeout_get(&ctimeout));
		if ( ret < 0 ) {
			if ( ret == -ETIMEDOUT ) {
				// timeout!
				return ret;
			}
			// -ENOBUFS
			// we still have time, we try again
			continue;
		}

		if ( msg ) *msg = uartbufrx_buf(t);
		return ret;
	}
	return 0;
}

/**
 * Prepare receive buffer to receive next message
 * @param t
 * @param msg
 * @param size
 */
void uartbufrx_findmsg_next(struct uartbufrx_s *t, size_t size)
{
	uartbufrx_FlushN(t, size?size:1);
}

/**
 * Fills buf which is buflen long with new message found conforming with conf in specified timeout
 * @param t
 * @param conf
 * @param buf
 * @param buflen
 * @param Timeout
 * @return
 */
int uartbufrx_getmsg(struct uartbufrx_s *t, const struct uartbufrx_findmsgconf_s * restrict conf,
						 uint8_t * restrict buf, size_t buflen, uint32_t Timeout)
{
	ssize_t ret = 0;
	const uint8_t *msg = NULL;
	if ( (ret = uartbufrx_findmsg(t, &msg, conf, NULL, Timeout)) >= 0 ) {
		if ( ret > buflen ) return -ENOBUFS;
		memcpy(buf, msg, ret);
		uartbufrx_findmsg_next(t, ret);
	}
	return ret;
}

/**
 * Compare minlen bytes from buf and (uint8_t *)arg
 * @param buf
 * @param minlen
 * @param arg
 * @return
 */
static int uartbufrx_waitformsg_checkBeginning(const uint8_t * restrict buf, size_t minlen, void *arg)
{
	const uint8_t *msg = arg;
	return !memcmp(buf, msg, minlen) ? minlen : 0;
}

/**
 * Wait for specified Timeout an exact message is received
 * @param t
 * @param msg
 * @param msgsize
 * @param Timeout
 * @return
 */
bool uartbufrx_waitformsg(struct uartbufrx_s * restrict t,
						  const uint8_t * restrict msg, size_t msgsize,
						  uint32_t Timeout)
{
	if ( uartbufrx_findmsg_beginning(t, msgsize,
									 uartbufrx_waitformsg_checkBeginning, (void*)msg,
									 Timeout) > 0 ) {
		uartbufrx_FlushN(t, msgsize);
		return true;
	}
	return false;
}


