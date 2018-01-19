/******************************************************************************

Welcome to GDB Online.
GDB online is an online compiler and debugger tool for C, C++, Python, PHP, HTML, CSS, JS
Code, Compile, Run and Debug online from anywhere in world.

*******************************************************************************/

#define _GNU_SOURCE
#include "uartbuf.h"
#include "timeout.h"
#include "hal.h"
#include "uartbuf_tests.h"

#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

int main()
{
//	uartbuftx_test1();
	uartbuftx_test2();
	return 0;
    uartbufrx_pthread_1_0();
    uartbufrx_pthread_1_1();
    uartbufrx_pthread_1_2();
    return 0;

    struct tests_s {
    	void (* const f)();
    	const char * const name;
    };
#define ADD(x) { &x, #x }
    struct tests_s tests[] = {
			ADD(uartbufrx_test_1),
			ADD(uartbufrx_test_2_findmsg_beginning),
			ADD(uartbufrx_test_3_findmsg_ending),
			ADD(uartbufrx_test_4_findmsg),
			ADD(uartbufrx_test2_test1),
			ADD(uartbufrx_test2_test2),
			ADD(uartbufrx_pthread_1_0),
			ADD(uartbufrx_pthread_1_1),
			ADD(uartbufrx_pthread_1_2),
	};
	for(size_t i = 0; i < sizeof(tests)/sizeof(tests[0]); ++i) {
		printf("TEST: Running: %s\n", tests[i].name);
		tests[i].f();
		printf("TEST: Run: %s\n", tests[i].name);
	}

	return 0;
}

