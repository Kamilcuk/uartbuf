The Uniwersal Asynchronous Receiver-Transmitter Buffered Library
================================================================

The universal library started out of need and boredom to give certain amount of simple functions to receive and send messages (not bytes) via a output function without using (m-)alloc() family functions.
Library was written under some influence of stm32 HAL library.

Prerequisites
===========

- C99 compatible compiler
- Written user customized library callbacks. Callbacks are used for example to send data via asynchronous manner, notify of buffer overflow, or enable or disable IRQ.
- timeout library. I have used in code some king of "timeout_t" object to handle timeout related operations. Typical function using timeout looks like this:
```
int some_function(int data, timeout_t *Timeout) {
	do {
		// do some work with data
		data ^= 0xff;
		if ( data == 0 ) 
			return EXIT_SUCCESS:
		// until timeout is expired
	} while( !timeout_expired(to) );
	return -ETIMEDOUT;
}
```
I pass timeout of uint32_t cause all in the HAL_ code uses that type.
User must implement `timeout_t` type, `timeout_t timeout_create(uint32_t)` function and `bool timeout_expired(timeout_t*)`.

uartbuftx
=========

uartbuftx library exists out of pure need to send data via uart using interrupt transfer and maybe optimize it with DMA. The printf()/puts() stdio functions outputted one character at a time on my system, which resulted in my need to buffer them and send in bigger chunks using DMA, so my processor could take on others, more sophisticated functions.

Data to send are send using uartbuftx_Write function. They are accumulated in an buffer. If there is no ongoing transmission, data are copied into the buffer and this buffer gets transmitted using interrupt transfer.
If there is an ongoing transmission, data are copied on the end of the buffer (after the data being currently transmitted). When transmission interrupt occurs, buffered data are being send.
If there is no space in the buffer, the functions wait's until as many characters are transferred as needed to start using our buffer.

User application may also not implement transmission completion interrupt in their application. Then the uartbuftx_s->use_interrupt flag should be set to 0, and uartbuftx_Write() function will just wait for the completion of the last write. (still, if there is no ongoing transmission, data will be copied into buffer and asynchronously transmitted)

uartbufrx
========

This module is rather hard to grasp and i will try to explain it here.

The module assumption is to use one fifo buffer layed out so that data always start at the beginning of the buffer. New data are being copyied to the end of the buffer, and then some end position indicator is being moved. When data in the buffer get used, the user should call uartbufrx_Flush() functions, to move buffer to the left specified number of bytes.

The receiving of data may occur in interrupt. The function uartbufrx_RxCplt_IRQHandler is used to notify, that data were received to some certain buffer position with certain length. Those data are then copied into uartbufrx_s->buf buffer. This function may be called from an IRQHandler, but it also may be not, if use wish to implement some kind of multi-threading inter-process communication.
The receiving data may also be transfered by DMA circuit directly into the uartbufrx_s->buf buffer. Then uartbufrx_RxCplt_DMA_IRQHandler should be called to notify the uartbufrx library of how many bytes were transferred into the buffer. uartbufrx library will request arming receiving interrupt by using a callback. User should use DMA or no DMA transfer depending on his needs.

The uartbufrx buffer pointer and length are obtained using specific functions. User may modify, clean and do anything he wants with that buffer up until that length. The rest of the buffer (after the "used" length) is used by incoming circuit/interrupt to input receiving data. To notify uartbufrx library that specific amount of data are no longer needed, user application should call uartbufrx_FlushN() function.


timeout_t library
==============

Stm32 in their HAL library all the time use the following code in multiple places to check for timeout:
```
static HAL_StatusTypeDef UART_WaitOnFlagUntilTimeout(UART_HandleTypeDef *huart, uint32_t Flag, FlagStatus Status, uint32_t Timeout)
{
  uint32_t tickstart = 0;

  /* Get tick */ 
  tickstart = HAL_GetTick();

  /* Wait until flag is set */
  if(Status == RESET)
  {
    while(__HAL_UART_GET_FLAG(huart, Flag) == RESET)
    {
      /* Check for the Timeout */
      if(Timeout != HAL_MAX_DELAY)
      {
        if((Timeout == 0)||((HAL_GetTick() - tickstart ) > Timeout))
        {
          /* some work when timeout expired */
          return HAL_TIMEOUT;
        }
      }
    }
    ...
}
```
This seems for me, like some code could be incorporated into some kind of HAL_TIMEOUT module:
```
static HAL_StatusTypeDef UART_WaitOnFlagUntilTimeout(UART_HandleTypeDef *huart, uint32_t Flag, FlagStatus Status, uint32_t Timeout)
{
  TIMEOUT_HandleTypeDef htimeout;

  /* Get tick */   
  htimeout = HAL_TIMEOUT_Start(Timeout);

  /* Wait until flag is set */
  if(Status == RESET)
  {
    while(__HAL_UART_GET_FLAG(huart, Flag) == RESET)
    {
      /* Check for the Timeout */
      if ( HAL_TIMEOUT_Expired(&htimeout) ) {
	  /* some work when timeout expired */
          return HAL_TIMEOUT;
      }
    }
  }
  ...
}
```
Where HAL_TIMEOUT module may be safely implemented as:
```
typedef {
	uint32_t tickstart;
	uint32_t Timeout;
} TIMEOUT_HandleTypeDef;

inline
TIMEOUT_HandleTypeDef HAL_TIMEOUT_Start(uint32_t Timeout) {
	TIMEOUT_HandleTypedef htimeout;
	htimeout.tickstart = HAL_GetTick();
	htimeout.Timeout = Timeout;
	return htimeout;
}
inline
int HAL_TIMEOUT_Expired(TIMEOUT_HandleTypeDef *htimeout) {
     if(htimeout->Timeout != HAL_MAX_DELAY)
      {
        if((htimeout->Timeout == 0)||((HAL_GetTick() - htimeout->tickstart ) > htimeout->Timeout))
        {
          return 1;
        }
      }
      return 0;
 }
```
Or force inline with STATIC_INLINE or macros:
```
#define HAL_TIMEOUT_Start(Timeout) (TIMEOUT_HandleTypeDef){.tickstart = HAL_GetTick(), .Timeout = Timeout}
#define HAL_TIMEOUT_Expired(htimeout)    ( \
    ((htimeout)->Timeout != HAL_MAX_DELAY ) && \
    ( \
      ((htimeout)->Timeout == 0 )||((HAL_GetTick() - (htimeout)->tickstart ) > (htimeout)->Timeout)) \
    )\
 )
```
With inline functions and a decent optimization this code should be equal to the original (not run as fast, but really equal).
Really, if HAL would incorporate this, they could use TIMEOUT_HandleTypeDef as argument to functions (which will allow counting on a cumulative timeout throughout multiple function calls) instead `uint32_t Timeout`.

Author
======

Written by Kamil Cukrowski 2017
Licensed jointly under the MIT License (opensource.org/licenses/mit-license.php) and the Beerware License (people.freebsd.org/∼phk/).

