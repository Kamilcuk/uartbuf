/*
 * uartbufrx_test2.c
 *
 *  Created on: 14 sty 2018
 *      Author: kamil
 */

#include <uartbuf.h>
#include <assert.h>
#include <unistd.h>
#include "uartbuf_tests.h"
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#define TEST(expr) assert(expr)

static enum {
	MODE_NORMAL = 0,
	MODE_DMA = 1,
	MODE_DMA_BLOCK = 2,
} mode = 0; // mode of operation
static pthread_mutex_t mirq; // represents IRQ from peripheral - enabled or disabled

// pointer and size obtained from ArmReceive function
static pthread_mutex_t mwritepnt;
static uint8_t *writepnt = NULL;
static size_t writepnt_size = 0;

static bool silent = true;

// count summary of received data
static volatile size_t sumread = 0;
static volatile size_t sumwritten = 0;

#define printf2(...)    do { if ( !silent ) { printf(__VA_ARGS__); } }while(0)
#define txprintf(msg, ...)   printf2(">%02lu: " msg, pthread_self()%10, ##__VA_ARGS__)
#define rxprintf(msg, ...)   printf2(" R%02lu: " msg, pthread_self()%10, ##__VA_ARGS__)
#define irqprintf(msg, ...)  printf2(" %02lu: " msg, pthread_self()%10, ##__VA_ARGS__)

static void t1_EnableIRQ(const struct uartbufrx_s *t)
{
	irqprintf("   %s(): %p\n", __func__, t);
	pthread_mutex_unlock(&mirq);
}
static void t1_DisableIRQ(const struct uartbufrx_s *t)
{
	irqprintf("   %s(): %p\n", __func__, t);
	pthread_mutex_lock(&mirq);
}
static bool t1_IsReceiving(const struct uartbufrx_s *t)
{
	if ( mode == 0 ) return true;
	pthread_mutex_lock(&mwritepnt);
	const bool ret = writepnt != NULL && writepnt_size != 0;
	pthread_mutex_unlock(&mwritepnt);
	return ret;
}
static void t1_ArmReceive(struct uartbufrx_s *t, uint8_t buf[], size_t size)
{
	if ( mode == 0 ) return;
	irqprintf("   %s(): %p %p %zu : %p %zu %d\n", __func__, t, buf, size,
			writepnt, writepnt_size, t1_IsReceiving(t));
	TEST( ! t1_IsReceiving(t) );
	TEST(buf == &t->buf[t->bufpos] && size == (t->bufsize - t->bufpos) );
	pthread_mutex_lock(&mwritepnt);
	writepnt = buf;
	writepnt_size = size;
	pthread_mutex_unlock(&mwritepnt);
}
static void t1_ArmReceive_IRQHandler(struct uartbufrx_s *t, uint8_t buf[], size_t size)
{
	if ( mode == 0 ) return;
	irqprintf("   %s(): %p %p %zu : %d\n", __func__, t, buf, size, pthread_mutex_trylock(&mirq));
	TEST( pthread_mutex_trylock(&mirq) == EBUSY );
	TEST(buf == &t->buf[t->bufpos] && size == t->bufsize - t->bufpos);
	writepnt = buf;
	writepnt_size = size;
}

static struct uartbufrx_s alloc_t = {
	.buf = ((uint8_t[256]){0}),
	.bufsize = 256,
	.DisableIRQ = t1_DisableIRQ,
	.EnableIRQ = t1_EnableIRQ,
	.ArmReceive = t1_ArmReceive,
	.ArmReceive_IRQHandler = t1_ArmReceive_IRQHandler,
	.IsReceiving = t1_IsReceiving,

};
static struct uartbufrx_s *t = &((struct uartbufrx_s){});

struct t1_s {
	struct uartbufrx_s *t;
	bool stop;
	size_t sumread;
	size_t sumwritten;
	uint8_t armpnt;
	size_t armpnt_size;
	int tx_mode;
};

#define t1_tx_rand_delay 0
#define t1_rx_rand_delay 2

static void *t1_tx(void *arg)
{
	sumwritten = 0;
	bool *stop = arg;
	for(int i=0; !*stop; usleep(rand()%t1_rx_rand_delay), ++i) {
		uint8_t buf[100];
		int buflen = snprintf((char*)buf, sizeof(buf), "$MESSAGE %d - %.0Lf#", i,  powl(10, rand()%15));

		switch(mode) {
		case 0:
			if ( uartbufrx_free(t) > buflen ) {
				sumwritten += buflen;
				t1_DisableIRQ(t);
				txprintf("Writing %d \"%s\"\n", buflen, buf);
				uartbufrx_RxCplt_IRQHandler(t, buf, buflen);
				t1_EnableIRQ(t);
			}
			break;
		case 2:
			while(!*stop && !t1_IsReceiving(t));
			/* no break */
		case 1: {
			if ( !t1_IsReceiving(t) ) {
				txprintf("Is not receiving...\n");
				break;
			}
			if ( writepnt_size > buflen ) {
				memcpy(writepnt, buf, buflen);
				sumwritten += buflen;
				t1_DisableIRQ(t);
				txprintf("Writing %d \"%s\"\n", buflen, buf);
				uartbufrx_RxCplt_DMA_IRQHandler(t, writepnt, buflen);
				t1_EnableIRQ(t);
			} else {
				txprintf("Stop receiving\n");
				writepnt = NULL;
				writepnt_size = 0;
			}
		}	break;
		}
	}
	return NULL;
}
static void *t1_rx(void *arg)
{
	sumread = 0;
	bool *stop = arg;
	for(int i=0; !*stop; usleep(rand()%t1_rx_rand_delay), ++i) {
		{
			volatile size_t bufsize = uartbufrx_size(t);
			volatile size_t buflen = uartbufrx_len(t);
			volatile size_t buffree = uartbufrx_free(t);
			rxprintf("Bufstat: %zu=%zu+%zu\n", bufsize, buflen, buffree);
		}
		char buf[100];
		size_t buflen = uartbufrx_len(t);
		if ( buflen == 0 ) { rxprintf("Read 0 chars\n"); continue; }
		if ( buflen > 99 ) { buflen = 99; }
		memcpy(buf, uartbufrx_buf(t), buflen);
		buf[buflen] = '\0';

		char *end = strchr(buf, '#');
		if ( end == NULL ) {
			rxprintf("NULL: %s\n", buf);
		}
		TEST(end != NULL);
		end[1] = '\0';
		buflen = end-buf+1;

		uartbufrx_FlushN(t, buflen);
		sumread += buflen;
		rxprintf("Reading %zu \"%s\"\n", buflen, buf);
	}

	usleep(200);
	size_t buflen = uartbufrx_len(t);
	rxprintf("Last Read: %zu\n", buflen);
	sumread += buflen;
	return NULL;
}

static void t1_run()
{
	pthread_t rx;
	pthread_t tx;
	bool stop = false;
	if ( pthread_mutex_init(&mwritepnt, NULL) ) { assert(0); }
	if ( pthread_mutex_init(&mirq, NULL) ) { assert(0); }
	if ( pthread_create(&rx, NULL, &t1_rx, &stop) ) { assert(0); }
	if ( pthread_create(&tx, NULL, &t1_tx, &stop) ) { assert(0); }
	sleep(2);
	stop = true;
	pthread_join(rx, NULL);
	pthread_join(tx, NULL);
	pthread_mutex_destroy(&mirq);
	pthread_mutex_destroy(&mwritepnt);

	printf("Sumread: %zu==%zu sumwritten\n", sumread, sumwritten);
	TEST(sumread == sumwritten);
}

void uartbufrx_pthread_1_0()
{
	memcpy(t, &alloc_t, sizeof(alloc_t));
	mode = 0;
	t1_run();
}

void uartbufrx_pthread_1_1()
{
	memcpy(t, &alloc_t, sizeof(alloc_t));
	mode = 1;
	t1_run();
}

void uartbufrx_pthread_1_2()
{
	memcpy(t, &alloc_t, sizeof(alloc_t));
	mode = 2;
	t1_run();
}
