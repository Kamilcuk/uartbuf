/*
 * uartbuftx_tests.c
 *
 *  Created on: 16 sty 2018
 *      Author: kamil
 */
#define _GNU_SOURCE
#include <uartbuf.h>
#include <assert.h>
#include <string.h>
#define TEST assert

static const uint8_t nums[] = "1234567890";
static const uint8_t abc[] = "abcdefghijklmnoprstuvxyz";
static const uint8_t ABC[] = "ABCDEFGHIJKLMNOPRSTUVXYZ";

void uartbuftx_test1_in(int mode)
{
	struct uartbuftx_s t_alloc = {
		.buf = ((uint8_t[10]){[0 ... 9] = ' '}),
		.size = 10,
	};
	struct uartbuftx_s *t = &t_alloc;

	printf("%s %d\n", __func__, mode);

	TEST(t->head == 0);

	uartbuftx_Write(t, nums, 1);
	uartbuftx_printf(t);
	TEST(t->buf[0] == nums[0] && t->head == 1);

	uartbuftx_TxCplt_IRQHandler(t);
	uartbuftx_printf(t);
	TEST(t->head == 1 && t->tail == 1 && t->txsize == 0);

	uartbuftx_Write(t, nums, 3);
	uartbuftx_printf(t);
	TEST(t->head == 3 && t->tail == 0 && t->txsize == 3 && !memcmp(&t->buf[0], nums, 3));

	uartbuftx_Write(t, abc, 5);
	uartbuftx_printf(t);
	TEST(t->head == 8 && t->tail == 0 && t->txsize == 3);

	uartbuftx_TxCplt_IRQHandler(t);
	uartbuftx_printf(t);
	TEST(t->head == 8 && t->tail == 3 && t->txsize == 5);

	if ( (mode&3) == 0 ) {
		uartbuftx_Write(t, nums, 3);
		uartbuftx_printf(t);
		TEST(t->head == 1 && t->tail == 3 && t->txsize == 5);

		uartbuftx_TxCplt_IRQHandler(t);
		uartbuftx_printf(t);
		TEST(t->head == 1 && t->tail == 8 && t->txsize == 2);

		uartbuftx_Write(t, ABC, 4);
		uartbuftx_printf(t);

		uartbuftx_TxCplt_IRQHandler(t);
		uartbuftx_printf(t);

		uartbuftx_TxCplt_IRQHandler(t);
		uartbuftx_printf(t);
	} else if ( (mode&3) == 1 ) {
		uartbuftx_Write(t, nums, 2);
		uartbuftx_printf(t);
		TEST(t->head == 0 && t->tail == 3 && t->txsize == 5);

		uartbuftx_TxCplt_IRQHandler(t);
		uartbuftx_printf(t);

		uartbuftx_Write(t, ABC, 4);
		uartbuftx_printf(t);

		uartbuftx_TxCplt_IRQHandler(t);
		uartbuftx_printf(t);
	}
}

void uartbuftx_test1()
{
	uartbuftx_test1_in(0);
	uartbuftx_test1_in(1);
}

#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <sched.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include "crc8.h"

static pthread_mutex_t mprintf = PTHREAD_MUTEX_INITIALIZER;
#define printf2(...)    do { if ( !s.silent ) { \
	pthread_mutex_lock(&mprintf); \
	printf(__VA_ARGS__); \
	pthread_mutex_unlock(&mprintf); \
} }while(0)
#define txprintf(msg, ...)   printf2(">%02lu:  " msg, pthread_self()%10, ##__VA_ARGS__)
#define rxprintf(msg, ...)   printf2(" R%02lu: " msg, pthread_self()%10, ##__VA_ARGS__)
#define irqprintf(msg, ...)  printf(" %02lu: " msg, pthread_self()%10, ##__VA_ARGS__)

struct tx_state_s {
	pthread_t rx, tx;

	volatile bool irq;
	const uint8_t * volatile buf;
	volatile size_t size;

	enum {
		MODE_SYNCHRONOUS = 0,
		MODE_ASYNCHRONOUS = 1,
	} volatile  mode; // mode of operation

	pthread_mutex_t mirq; // represents IRQ from peripheral - enabled or disabled

	bool silent;
	volatile size_t sumread;
	volatile size_t sumwritten;

	pthread_mutex_t msingle_core;

	volatile bool stop;

	unsigned char rxcrc;
	unsigned char txcrc;

	bool tx_ended;
};
static struct tx_state_s s = {
	.silent = false,
	.msingle_core = PTHREAD_MUTEX_INITIALIZER,
};

static void txt1_EnableIRQ(const struct uartbuftx_s *t)
{
//	irqprintf("   %s(): %p\n", __func__, t);
	pthread_mutex_unlock(&s.mirq);
}
static void txt1_DisableIRQ(const struct uartbuftx_s *t)
{
//	irqprintf("   %s(): %p\n", __func__, t);
	pthread_mutex_lock(&s.mirq);
}
static void txt1_Write(struct uartbuftx_s *t, const uint8_t buf[], size_t size)
{
	irqprintf("   %s(): %p %p %zu\n", __func__, t, buf, size);
	if(size == 0) return;
//	assert(s.size != 0);
	s.irq = false;
	s.buf = buf;
	s.size = size;
	irqprintf("   OUT %s(): %p %p %zu\n", __func__, t, buf, size);
}
static void txt1_Write_IRQHandler(struct uartbuftx_s *t, const uint8_t buf[], size_t size)
{
	printf("   %s(): %p %p %zu\n", __func__, t, buf, size);
	assert(s.size == 0);
	if(size == 0) return;
	s.irq = false;
	s.buf = buf;
	s.size = size;
	irqprintf("   OUT %s(): %p %p %zu\n", __func__, t, buf, size);
}
static void txt1_Flush(const struct uartbuftx_s *t)
{
	irqprintf("   %s(): %p\n", __func__, t);
}

static struct uartbuftx_s t_alloc = {
	.buf = ((uint8_t[80]){0}),
	.size = 80,
	.EnableIRQ = txt1_EnableIRQ,
	.DisableIRQ = txt1_DisableIRQ,
	.Write = txt1_Write,
	.Write_IRQHandler = txt1_Write_IRQHandler,
	.Flush = txt1_Flush,
};
static struct uartbuftx_s *t = &t_alloc;


#define t1_tx_rand_delay 10
#define t1_rx_rand_delay 20

static void *txt1_tx(void *arg)
{
	s.tx_ended = false;
	s.sumwritten = 0;
	for(int i=0; !s.stop; usleep(rand()%t1_tx_rand_delay), ++i) {
		uint8_t buf[100];
		const int buflen = snprintf((char*)buf, sizeof(buf), "$MESSAGE %d - %.0Lf#", i,  powl(10, rand()%15));

		txprintf("Send: %d %s\n", buflen, buf);
		s.txcrc = append_to_crc8(s.txcrc, buf, buflen);
		s.sumwritten += buflen;
		uartbuftx_Write(t, buf, buflen);

		pthread_mutex_lock(&mprintf);
		if ( !s.silent ) uartbuftx_printf(t);
		pthread_mutex_unlock(&mprintf);
	}
	printf("%s END\n", __func__);
	s.tx_ended = true;
	return NULL;
}

static void txt1_rx_SIGUSR1_handler(int ret)
{
	s.size = 0;
	s.buf = NULL;

	// simulate single core processor by stopping the other thread
	memset(&t->buf[t->tail], ' ', t->txsize);
	uartbuftx_TxCplt_IRQHandler(t);
}

static void *txt1_rx(void *arg)
{
	s.sumread = 0;
	for(int i=0; !s.stop || s.size != 0 || !s.tx_ended; ++i) {
		if ( s.size != 0 ) {
			assert(s.buf != NULL);
			usleep(rand()%t1_rx_rand_delay);
			uint8_t buf[300];
			memcpy(buf, s.buf, s.size);
			size_t size = s.size;
			s.sumread += size;
			buf[size] = '\0';
			assert(size+1 < sizeof(buf));
			s.rxcrc = append_to_crc8(s.rxcrc, buf, size);
			rxprintf("Recv: %2zu %s\n", size, buf);

			// call txt1_rx_SIGUSR1_handler
			pthread_mutex_lock(&mprintf);
			uartbuftx_DisableIRQ_Callback(t);
			raise(SIGUSR1);
			uartbuftx_EnableIRQ_Callback(t);
			pthread_mutex_unlock(&mprintf);
		}
	}
	printf("%s END %zu %zu %zu\n", __func__, t->head, t->tail, t->txsize);
	return NULL;
}


void uartbuftx_test2()
{
	signal(SIGUSR1, txt1_rx_SIGUSR1_handler);

	if ( pthread_mutex_init(&s.mirq, NULL) ) { assert(0); }
	if ( pthread_create(&s.rx, NULL, &txt1_rx, NULL) ) { assert(0); }
	if ( pthread_create(&s.tx, NULL, &txt1_tx, NULL) ) { assert(0); }
	printf("STOPPING THREADS 1\n");
	sleep(2);
	s.stop = true;
	printf("STOPPING THREADS 2\n");
	pthread_join(s.rx, NULL);
	pthread_join(s.tx, NULL);
	pthread_mutex_destroy(&s.mirq);

	signal(SIGUSR1,  NULL);

	printf("sumread = %zu = %zu = sumwritten\n", s.sumread, s.sumwritten);
	printf("rxcrc = %02x = %02x = txcrc\n", s.rxcrc, s.txcrc);
}
