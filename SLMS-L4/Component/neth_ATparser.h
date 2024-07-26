/*
 * neth_ATparser.h
 *
 *  Created on: Aug 18, 2020
 *      Author: Saifa-NB
 */

#ifndef INC_NETH_ATPARSER_H_
#define INC_NETH_ATPARSER_H_

#ifdef __cplusplus
extern "C"{
#endif
/*****************************************/
/* I N C L U D E                         */
/*****************************************/
#include "main.h"
#include "usart.h"

#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/*****************************************/
/* D E F I N I T I O N                   */
/*****************************************/
#define AT_CORE_BUFFER_SIZE     1500U /* 1024 - 1 */
#define AT_SET_ZERO             0
#define AT_RESPONSE_OFFSET      2    /* first \001\r\n is ignore */

#if(AT_SET_ZERO)
#error AT SET ZERO must be ZERO.
#endif

#if(AT_RESPONSE_OFFSET != 2)
#error AT RESPONSE OFFSET must be 2.
#endif



#define AT_DEBUG_MODE           0
#if(AT_DEBUG_MODE)
#define DBG_UART		   huart3
#define dbg_print(...)     HAL_UART_Transmit(&DBG_UART, (uint8_t*)__VA_ARGS__, strlen((char*)__VA_ARGS__), 1000)
#define dbg_println(...)   HAL_UART_Transmit(&DBG_UART, (uint8_t*)__VA_ARGS__, strlen((char*)__VA_ARGS__), 1000);\
						   HAL_UART_Transmit(&DBG_UART, (uint8_t*)"\r\n", 2, 1000)
#define dbg_println_size(size, ...)   HAL_UART_Transmit(&DBG_UART, (uint8_t*)__VA_ARGS__, size, 1000);\
						              HAL_UART_Transmit(&DBG_UART, (uint8_t*)"\r\n", 2, 1000)
#endif
#if(!AT_DEBUG_MODE)
#define DBG_UART		   huart3
#define dbg_print(...)
#define dbg_println(...)
#define dbg_println_size(size, ...)
#endif

/*****************************************/
/* E N U M U L A T O R                   */
/*****************************************/
typedef enum{
	AT_TIMEOUT_500MS   = 500U,
	AT_TIMEOUT_1000MS  = 1000U,
	AT_TIMEOUT_2000MS  = 2000U,
	AT_TIMEOUT_5000MS  = 5000U,
	AT_TIMEOUT_10000MS = 10000U,
	AT_TIMEOUT_20000MS = 20000U,
	AT_TIMEOUT_15000MS = 15000U,
	AT_TIMEOUT_30000MS = 30000U /* IWDG can reset system */
}AT_Timeout;

typedef enum{
	AT_RESPONSE_OK = 0,
	AT_RESPONSE_ERROR,
	AT_RESPONSE_IDLE
}AT_Response;

typedef union {
    struct {
        unsigned usartRxError: 1;
        unsigned FlagRDY: 1;
        unsigned FlagOK: 1;
        unsigned FlagError: 1;
        unsigned FlagCRLF: 1;
        unsigned FlagEndCmd: 1;
        unsigned FlagResponse: 1;
        unsigned FlagURC: 1;
    };
    uint8_t AllFlags;
}AT_Flag_t;

typedef struct{
	uint32_t  		LastTimeRecieved;
	uint8_t 		ByteRecv;    /* Current received data byte */
	uint8_t 		RxBuffer[AT_CORE_BUFFER_SIZE];
	uint8_t 		TxBuffer[AT_CORE_BUFFER_SIZE];
	uint16_t		RxIndex;     /* Index of receiver buffer */
	AT_Flag_t       Flags;
}AT_t;


/*****************************************/
/* PROTOTYPE FUNCTION (Global)           */
/*****************************************/
bool AT_init(UART_HandleTypeDef *huart);
void AT_RxCallback(void);

/*****************************************/
/* PROTOTYPE FUNCTION (Static)           */
/*****************************************/
bool AT_sendCheckReply(uint8_t *toSend, uint8_t *reply, uint32_t timeOut);
bool AT_sendNoCheck(uint8_t *send);
bool AT_waitForString(uint8_t *result, uint32_t timeOut, uint8_t CountOfParameter, ...);
bool AT_returnString(uint8_t *result,uint8_t WantWhichOne,uint8_t *SplitterChars);
uint16_t AT_readline(int16_t timeout, bool multiline);
uint16_t AT_readlineOK(int16_t timeout);
uint8_t AT_getReply(uint8_t *send, uint16_t timeout);
bool AT_parseReply(uint8_t* toreply, uint32_t *v, uint8_t divider, uint8_t index);
bool AT_sendParseReply(uint8_t* tosend, uint8_t* toreply, uint32_t *v, uint8_t divider, uint8_t index);
bool AT_expectReplyOK(uint8_t *send, uint16_t timeout);
bool AT_sendCheckReplyByte(uint8_t *send, uint8_t *need, uint16_t timeout);
bool AT_sendCommand(uint8_t *toSend);
void AT_catchOK(void);
void AT_catchError(void);
void AT_catchRDY(void);
void AT_TxClear(void);
void AT_RxClear(void);
void AT_ClearAllFlag(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_NETH_ATPARSER_H_ */
