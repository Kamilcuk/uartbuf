/*
 * uartbuf_callbacks.c
 *
 *  Created on: 31 gru 2017
 *      Author: kamil
 */

struct ubrx_

void uartbufrx_EnableIRQ_Callback(const struct uartbufrx_s *t)
{

}
void uartbufrx_DisableIRQ_Callback(const struct uartbufrx_s *t) __nonnull((1));
void uartbufrx_Receive_IT_Callback(const struct uartbufrx_s *t, uint8_t buf[], size_t size) __nonnull((1,2));
void uartbufrx_IsOverflowed_Callback(struct uartbufrx_s *t) __nonnull((1));
