/*
 * cumulativetimeout.h
 *
 *  Created on: 21.12.2016
 *      Author: kamil
 */
#include "timeout.h"

#include <time.h>

static
clock_t msclock()
{
	return clock()/(CLOCKS_PER_SEC/1000);
}

timeout_t timeout_create(clock_t Timeout)
{
	timeout_t ret = { .Timeout = Timeout };
	if (!( Timeout == 0 || Timeout == TIMEOUT_MAX )) {
		ret.Tickstart = msclock();
	}
	return ret;
}

clock_t timeout_get(timeout_t *this)
{
	if ( this->Timeout == 0 || this->Timeout == TIMEOUT_MAX ) {
		return this->Timeout;
	}
	const clock_t now = msclock();
	const clock_t Tickstop = this->Tickstart + this->Timeout;
	if ( now < Tickstop ) {
		return 0;
	}
	return Tickstop - now;
}
