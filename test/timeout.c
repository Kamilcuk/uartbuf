/*
 * cumulativetimeout.h
 *
 *  Created on: 21.12.2016
 *      Author: kamil
 */
#include "timeout.h"

#include "hal.h" // HAL_GetTick()

inline
timeout_t timeout_create(uint32_t Timeout)
{
	if ( Timeout == 0 || Timeout == UINT32_MAX )
		return (timeout_t){Timeout};
	const uint32_t Ticknow = HAL_GetTick();
	uint32_t Tickstop = Timeout + Ticknow;
	if ( Tickstop == 0 || Tickstop == UINT32_MAX ) {
		// we will wait a little longer
		// however this will occur NEVER
		++Tickstop;
	}
	return (timeout_t){ .Tickstop = Tickstop };
}

inline
uint32_t timeout_get(timeout_t *this)
{
	if ( this->Tickstop == 0 || this->Tickstop == UINT32_MAX ) return this->Tickstop;
	const uint32_t Ticknow = HAL_GetTick();
	if ( this->Tickstop < Ticknow ) {
		this->Tickstop = 0;
		return 0;
	}
	return this->Tickstop - Ticknow;
}

inline
bool timeout_expired(timeout_t *this)
{
	return timeout_get(this) == 0;
}

