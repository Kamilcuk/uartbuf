
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
        uint32_t Tickstop;
} timeout_t;

timeout_t timeout_create(uint32_t Timeout);
uint32_t timeout_get(timeout_t *this);
bool timeout_expired(timeout_t *this);

#endif
