
#include <time.h>
#include "hal.h"
#include <assert.h>

/**
 * clock in miliseconds and in uint32_t
 * @return
 */
uint32_t HAL_GetTick() {
	static clock_t start = 0;
	const clock_t c = clock();
	const clock_t ms = c/(CLOCKS_PER_SEC/1000);
	if ( start == 0 ) start = ms;
	return (uint32_t)( ms - start );
}

