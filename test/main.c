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

#ifdef __WIN32
#include <windows.h>
void sleep(int x) { Sleep(1000 * x); }
#endif

struct uartbufrx_s *t = &UARTBUFRX_INIT(((uint8_t[256]){0}), 256);


#define dbg(str, ...) printf("%3u:%s(): " str "\n" , HAL_GetTick() , __FUNCTION__ , ##__VA_ARGS__)
#define dbgarr(start, how, arr, size, stop, ...)   do { \
        printf("%3u:%s(): " start , HAL_GetTick() , __FUNCTION__ , ##__VA_ARGS__); \
        for(int _i = 0; _i < (size); ++_i) { \
                printf( (how), (arr)[_i] ); \
        } \
        printf(stop); \
} while(0)

struct DMA_s {
        uint8_t *buf;
        size_t size;
};
struct DMA_s DMA = {0};

#if 0
void uartbufrx_Receive_IT_Callback(const struct uartbufrx_s *t, uint8_t *buf, size_t size)
{
        static struct timeout_s a = {0};
        if ( !timeout_expired(&a) ) return;
        a = timeout_create(10);
        int type = t->priv.type;
        dbg("");
        switch(type) {
                case 0: break;
                case 1:
                        DMA.buf = buf; DMA.size = size;
                        break;
                default: assert(0); break;
        }
}
#endif

void thread_rx_10milis()
{
        static uint8_t input[] = "1234567890qwertyuiopasdfghjklzxcvbnm";
        static int inputpos = 0;
        int chunk = 10;
        if ( inputpos >= sizeof(input) ) return;

        uint8_t * const in = &input[inputpos];
        const size_t insize = chunk;

        dbg("%d", inputpos);
//        struct arg_s * const arg = t->priv.arg;
        int type = 0;
        switch(type) {
                case 0: break;
                case 1:
                        uartbufrx_RxCplt_IRQHandler((void*)t,in,insize);
                        break;
                case 2:
                        chunk = DMA.size;
                        memcpy(DMA.buf, in, chunk);
                        uartbufrx_RxCplt_DMA_IRQHandler((void*)t, chunk);
                        break;
        }

        inputpos += chunk;
}

void *thread_rx(void *arg)
{
        dbg("");
        struct timeout_s a = timeout_create(100);
        while(!timeout_expired(&a)) {
                struct timeout_s b = timeout_create(10);
                while(!timeout_expired(&b)) {pthread_yield();}
                thread_rx_10milis();
        }
        return NULL;
}
void *thread_read(void *arg)
{
        dbg("");
        for(;;) {
                uint8_t buf[20];
                size_t read = uartbufrx_Read(t, buf, sizeof(buf), 1000);
                dbgarr("Read[%zd]: ", "%c", buf, read>0?read:0, "\n", read);
        }
        return NULL;
}



int main()
{
    setvbuf(stdout, NULL, _IOLBF, 100);
    setvbuf(stderr, NULL, _IOLBF, 100);
    fprintf(stderr, "stderr\n");
    fprintf(stdout, "stdout\n");

    int (* const tests[])() = {
			&uartbufrx_test_1,
			&uartbufrx_test_2_findmsg_beginning,
			&uartbufrx_test_3_findmsg_ending,
			&uartbufrx_test_4_findmsg,
	};
	for(int (* const *testi)() = &tests[0];
		testi < &tests[sizeof(tests)/sizeof(tests[0])];
		++testi) {
		int (* const test)() = *testi;
		if ( test() != 0 ) {
			assert(0);
		}
	}

	return 0;
    pthread_t pthread_rx, pthread_read;
    if(pthread_create(&pthread_rx, NULL, &thread_rx, NULL))
    {
        perror("pthread_create");
        return 1;
    }
    if(pthread_create(&pthread_read, NULL, &thread_read, NULL))
    {
        perror("pthread_create");
        return 1;
    }
    sleep(1);
    return 0;
}

