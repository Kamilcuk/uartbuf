
/*
 * cumulativetimeout.h
 *
 *  Created on: 21.12.2016
 *      Author: kamil
 */

#ifndef CUMULATIVETIMEOUT_H_
#define CUMULATIVETIMEOUT_H_

#include <stdint.h>
#include <stdbool.h>

/*
 * All Timeout values are in miliseconds
 */

/**
 * Maximum timeout
 */
#define TIMEOUT_MAX  UINT32_MAX

typedef struct timeout_s {
	clock_t Tickstart;
	clock_t Timeout;
} timeout_t;

#define TIMEOUT_INIT(_Timeout) { .Tickstart = clock()/(CLOCKS_PER_SEC/1000), .Timeout = _Timeout}
timeout_t timeout_create(clock_t Timeout);
#define timeout_init(_this, _Timeout)          do{ *(_this) = timeout_create(_Timeout); }while(0)
#define timeout_deinit(_this)
clock_t timeout_get(timeout_t *this);
#define timeout_expired(_this) ( timeout_get(this) == 0 )

#endif
