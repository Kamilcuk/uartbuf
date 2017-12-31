
#include "uartbuf_tests.h"

#include "uartbuf.h"
#include "uartbufrx.h"
#include "_uartbuf.h"
#include "hal.h"

#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <inttypes.h>

#define udr_print(t) \
	printf("%"PRIu32":%d:[%d]: ", HAL_GetTick(), __LINE__, uartbufrx_buflen(t)); for(int i=0;i<uartbufrx_buflen(t);++i) { printf("%c", uartbufrx_buf(t)[i]); } printf("\n");

#define PRINTARR(start, how, arr, size, stop, ...)   do { \
	printf("%"PRIu32":%s:" start, HAL_GetTick(), __FUNCTION__, ##__VA_ARGS__); \
	for(int _i = 0; _i < (size); ++_i) { \
	printf( (how), (arr)[_i] ); \
	} \
	printf(stop); \
	} while(0)

#define TEST(expr) assert(expr)

int uartbufrx_test_1()
{
    struct uartbufrx_s *t = &UARTBUFRX_INIT(((uint8_t[256]){0}), 256);
    const uint8_t bufin[] = "1234567890a";
    uint8_t buf[sizeof(bufin)];

    {
        uartbufrx_RxCplt_IRQHandler(t, bufin, 10 );
        TEST( uartbufrx_buflen(t) == 10 );
        uartbufrx_FlushAll(t);
        TEST( uartbufrx_buflen(t) == 0 );
    }
    {
        uartbufrx_RxCplt_IRQHandler(t, bufin, 10 );
		TEST( uartbufrx_buflen(t) == 10 );
        TEST( !memcmp( uartbufrx_buf(t), bufin, 10) );
        uartbufrx_FlushN(t, 5);
        TEST( !memcmp( uartbufrx_buf(t), &bufin[5], 5) );
        uartbufrx_RxCplt_IRQHandler(t, &bufin[0], 5 );
        TEST( !memcmp( uartbufrx_buf(t), &bufin[5], 5) );
        TEST( !memcmp( &uartbufrx_buf(t)[5] , &bufin[0], 5) );
        TEST( uartbufrx_buflen(t) == 10 );
        uartbufrx_FlushN(t, 5);
        TEST( !memcmp( &uartbufrx_buf(t)[0] , &bufin[0], 5) );
        TEST( uartbufrx_buflen(t) == 5 );
        uartbufrx_FlushN(t, 5);
        TEST( uartbufrx_buflen(t) == 0 );
    }
    {
        uartbufrx_RxCplt_IRQHandler(t, bufin, 10 );
        uartbufrx_Read(t, buf, 10, 0);
        TEST( !memcmp(bufin, buf, 10 ) );
        TEST( uartbufrx_buflen(t) == 0 );
    }
    {
        uartbufrx_RxCplt_IRQHandler(t, bufin, 10 );
		uartbufrx_Read(t, buf, 10, 0 );
        TEST( !memcmp(bufin, buf, 10 ) );
        TEST( uartbufrx_buflen(t) == 0 );
    }
    {
        TEST( uartbufrx_bufsize(t) >= 20 ); // test for more than 20 chars
        // non DMA transfer into RxCplt
        uartbufrx_RxCplt_IRQHandler(t, bufin, 10); // 10 in
        TEST( uartbufrx_buflen(t) == 10 );
        // DMA transfer into uartbufrx_buf
        memcpy(&uartbufrx_buf_nonconst(t)[10], bufin, 10); // DMA transfer 10 in
        // inform we have transfered DMA data into t->buf
        uartbufrx_RxCplt_DMA_IRQHandler(t, 10); // notify of 10 in
        TEST( uartbufrx_buflen(t) == 20 );
        TEST( !memcmp( &uartbufrx_buf(t)[0], &bufin[0], 10) );
        TEST( !memcmp( &uartbufrx_buf(t)[10], &bufin[0], 10) );
        uartbufrx_FlushAll(t);
    }
    {
        TEST( uartbufrx_bufsize(t) >= 20 ); // test for more than 20 chars
        // non DMA transfer into RxCplt
        uartbufrx_RxCplt_IRQHandler(t, bufin, 10); // 10 in
        TEST( uartbufrx_buflen(t) == 10 );
        // DMA transfer into uartbufrx_buf
        memcpy(&uartbufrx_buf_nonconst(t)[10], bufin, 10); // DMA transfer 10 in
        // get 5 data
        uartbufrx_FlushN(t, 5);
        TEST( uartbufrx_buflen(t) == 5 );
        // inform we have transfered DMA data into t->buf
        uartbufrx_RxCplt_DMA_IRQHandler(t, 10); // notify of 10 in
        TEST( uartbufrx_buflen(t) == 15 );
        TEST( !memcmp( &uartbufrx_buf(t)[0], &bufin[5], 5) );
        TEST( !memcmp( &uartbufrx_buf(t)[5], &bufin[0], 10) );
        uartbufrx_FlushAll(t);
    }
    return 0;
}

struct uartbufrx_test_findmsg_arg_s {
	int checkBeginning_cnt;
	int checkEnding_cnt;
};

static
int uartbufrx_test_findmsg_checkBeginning(const uint8_t *buf, size_t size, void *arg)
{
	struct uartbufrx_test_findmsg_arg_s *a = arg;
	a->checkBeginning_cnt++;
	PRINTARR("buf:[%zd]=", "%c", buf, size, "\n", size);
	if ( buf[0] == '[' ) return buf[1] - '0';
	return 0;
}

static
int uartbufrx_test_findmsg_checkEnding(const uint8_t *buf, size_t size, void *arg)
{
	struct uartbufrx_test_findmsg_arg_s *a = arg;
	a->checkEnding_cnt++;
	PRINTARR("buf:[%zd]=", "%c", buf, size, "\n", size);
	if ( buf[size-1] == ']' ) return size; // message ok
	if ( buf[size-1] == '}' ) return -1; // CRC error
	return 0;
}

int uartbufrx_test_2_findmsg_beginning()
{
	const struct uartbufrx_findmsgconf_s conf = {
		.minMsgLen = 2,
				.maxMsgLen = 10,
				.checkBeginning = &uartbufrx_test_findmsg_checkBeginning,
				.checkEnding = &uartbufrx_test_findmsg_checkEnding,
	};
	struct uartbufrx_s *t = &UARTBUFRX_INIT(((uint8_t[256]){0}), 256);
	const uint8_t bufin[] = "abcdef[2234567890]abcdef[3234567}abcdef";
	int ret;

	{
		uartbufrx_FlushAll(t);
		struct uartbufrx_test_findmsg_arg_s arg = {0};
		uartbufrx_RxCplt_IRQHandler(t, bufin, sizeof(bufin));
		ret = uartbufrx_findmsg_beginning(t,
			conf.minMsgLen, conf.checkBeginning, &arg, 0 );
		TEST(ret == 2);
		TEST(arg.checkBeginning_cnt == 7);
		ret = uartbufrx_findmsg_beginning(t,
			conf.minMsgLen, conf.checkBeginning, &arg, 0 );
		TEST(ret == 2);
		TEST(arg.checkBeginning_cnt == 8);
	}
	{
		uartbufrx_FlushAll(t);
		struct uartbufrx_test_findmsg_arg_s arg = {0};
		uartbufrx_RxCplt_IRQHandler(t, bufin, 5);
		ret = uartbufrx_findmsg_beginning(t,
			conf.minMsgLen, conf.checkBeginning, &arg, 0 );
		TEST(ret == -ETIMEDOUT);
		TEST(arg.checkBeginning_cnt == 4);
		uartbufrx_RxCplt_IRQHandler(t, &bufin[5], 5);
		ret = uartbufrx_findmsg_beginning(t,
			conf.minMsgLen, conf.checkBeginning, &arg, 0 );
		TEST(ret == 2);
		TEST(arg.checkBeginning_cnt == 7);
	}
	return 0;
}

int uartbufrx_test_3_findmsg_ending()
{
	const struct uartbufrx_findmsgconf_s conf = {
		.minMsgLen = 2,
				.maxMsgLen = 10,
				.checkBeginning = &uartbufrx_test_findmsg_checkBeginning,
				.checkEnding = &uartbufrx_test_findmsg_checkEnding,
	};
	struct uartbufrx_s *t = &UARTBUFRX_INIT(((uint8_t[256]){0}), 256);
	const uint8_t bufin[] = "abcdef[2234567890]abcdef[3234567}abcdef";
	int ret;

	{
		uartbufrx_FlushAll(t);
		struct uartbufrx_test_findmsg_arg_s arg = {0};
		const uint8_t bufin2[] = "123[123]123";
		uartbufrx_RxCplt_IRQHandler(t, bufin2, sizeof(bufin2));
		ret = uartbufrx_findmsg_ending(t, 2,
			20, conf.checkEnding, &arg, 0 );
		printf("%d %d \n", ret, arg.checkEnding_cnt);
	}
	{
		uartbufrx_FlushAll(t);
		struct uartbufrx_test_findmsg_arg_s arg = {0};
		uartbufrx_RxCplt_IRQHandler(t, bufin, 12);
		ret = uartbufrx_findmsg_ending(t, 2,
			10, conf.checkEnding, &arg, 0 );
		TEST(ret == -ENOBUFS);
		TEST(arg.checkEnding_cnt == 9);

		arg.checkEnding_cnt = 0;
		ret = uartbufrx_findmsg_ending(t, 2,
			20, conf.checkEnding, &arg, 0 );
		TEST(ret == -ETIMEDOUT);
		TEST(arg.checkEnding_cnt == 11); // 12 chars inbuffer - ((2)-1)

		arg.checkEnding_cnt = 0;
		uartbufrx_RxCplt_IRQHandler(t, &bufin[12], 10);
		ret = uartbufrx_findmsg_ending(t, 2,
			21, conf.checkEnding, &arg, 0 );
		TEST(ret == 18);
		TEST(arg.checkEnding_cnt == 17);
	}

	return 0;
}


int uartbufrx_test_4_findmsg()
{
	const struct uartbufrx_findmsgconf_s conf = {
				.minMsgLen = 2,
				.maxMsgLen = 20,
				.checkBeginning = &uartbufrx_test_findmsg_checkBeginning,
				.checkEnding = &uartbufrx_test_findmsg_checkEnding,
	};
	struct uartbufrx_s *t = &UARTBUFRX_INIT(((uint8_t[256]){0}), 256);
	const uint8_t bufin[] = "abcdef[51234]abcdefg[5123456}abcdef";
	int ret;

	{
		uartbufrx_FlushAll(t);
		struct uartbufrx_test_findmsg_arg_s arg = {0};
		uartbufrx_RxCplt_IRQHandler(t, bufin, sizeof(bufin)-1);
		ret = uartbufrx_findmsg(t, NULL, &conf, &arg, 0 );
		TEST(ret == 7);
		TEST(arg.checkBeginning_cnt == 7);
		TEST(arg.checkEnding_cnt == 3);
		memset(&arg, 0, sizeof(arg));
		uartbufrx_findmsg_next(t, ret);
		ret = uartbufrx_findmsg(t, NULL, &conf, &arg, 0 );
		printf("%d %d %d \n", ret, arg.checkBeginning_cnt, arg.checkEnding_cnt);
	}
	return 0;
}
