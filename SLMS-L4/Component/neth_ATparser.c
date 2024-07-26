/*
 * neth_ATparser.c
 *
 *  Created on: Aug 18, 2020
 *      Author: Saifa-NB
 */


#include "neth_ATparser.h"

static UART_HandleTypeDef *UartInst = NULL;
static USART_TypeDef *USART_INST = NULL;
static uint8_t _State_OK;
static uint8_t _State_ERROR;
static uint8_t _State_RDY;

volatile AT_t _STORAGE;


/////////////////////////////////////////////////////////////////////////
// Public
/////////////////////////////////////////////////////////////////////////

bool AT_init(UART_HandleTypeDef *huart){
	UartInst = huart;
	USART_INST = huart->Instance;

	if(UartInst == NULL){
		return false;
	}

	/* Initialize buffer */
	memset((char*)_STORAGE.TxBuffer, AT_SET_ZERO, AT_CORE_BUFFER_SIZE);
	memset((char*)_STORAGE.RxBuffer, AT_SET_ZERO, AT_CORE_BUFFER_SIZE);
	_STORAGE.RxIndex             = AT_SET_ZERO;

	/* Flags */
	_STORAGE.Flags.AllFlags = AT_SET_ZERO;

	/* State Counter */
	_State_ERROR   = AT_SET_ZERO;
	_State_OK      = AT_SET_ZERO;

	/* interrupt disable and restart */
	//HAL_UART_Abort_IT(UartInst);
	__HAL_UART_ENABLE_IT(UartInst, UART_IT_RXNE);	/* Receive data */
	__HAL_UART_ENABLE_IT(UartInst, UART_IT_IDLE);  /* BUS IDLE check */

	return true;
}

//#########################################################################################################
void AT_RxCallback(void){
	/* If received new data byte */
	if(__HAL_UART_GET_IT(UartInst,UART_IT_RXNE)){
		_STORAGE.ByteRecv = (uint8_t)(USART_INST->RDR);
//		DBG_UART.Instance->TDR = _STORAGE.ByteRecv;

		AT_catchOK();
		AT_catchError();
//		AT_catchRDY();

		if(_STORAGE.RxIndex > AT_CORE_BUFFER_SIZE){
			_STORAGE.Flags.usartRxError = true;
		}else{
			_STORAGE.RxBuffer[_STORAGE.RxIndex++] = _STORAGE.ByteRecv;

		}
	}

	/* Idle flag set, all bytes received */
	if(__HAL_UART_GET_IT(UartInst, UART_IT_IDLE)){
		__HAL_UART_CLEAR_IDLEFLAG(UartInst);

//		HAL_UART_Transmit(&DBG_UART, _STORAGE.RxBuffer, (_STORAGE.RxIndex-1), 1000);

		/* If FlagEndCmd set by user -> Received response */
		_STORAGE.Flags.FlagResponse = true;

		/* If FlagEndCmd not set -> Received URC message */
		_STORAGE.Flags.FlagURC = true;

		/* Disable FlagEndCmd after used */
		_STORAGE.Flags.FlagEndCmd = false;

		/* Error Detection */
		_STORAGE.Flags.usartRxError |= __HAL_UART_GET_FLAG(UartInst, UART_FLAG_ORE);
		if(_STORAGE.Flags.usartRxError){
			HAL_UART_Abort_IT(UartInst);
			HAL_Delay(10);
			__HAL_UART_DISABLE_IT(UartInst, UART_IT_RXNE);
			__HAL_UART_DISABLE_IT(UartInst, UART_IT_IDLE);
			HAL_UART_Abort_IT(UartInst);
			HAL_Delay(10);
			__HAL_UART_ENABLE_IT(UartInst, UART_IT_RXNE);
			__HAL_UART_ENABLE_IT(UartInst, UART_IT_IDLE);
			_STORAGE.Flags.usartRxError = false;
		}
	}
}

//#########################################################################################################
bool AT_sendCheckReply(uint8_t *toSend, uint8_t *reply, uint32_t timeOut)
{
    bool ret = false;
	uint8_t result;

    AT_RxClear();
    AT_TxClear();
    AT_ClearAllFlag();

    sprintf((char*)_STORAGE.TxBuffer,"%s\r\n", (char*)toSend);
#if(AT_DEBUG_MODE)
    	dbg_print("\t---> "); dbg_println(toSend);
#endif

    if(AT_sendCommand((uint8_t*)_STORAGE.TxBuffer) == false){
        ret = false;
    }

    if(AT_waitForString((uint8_t*)&result, timeOut, 1, reply) == true){ // TODO Saifa Code
#if(AT_DEBUG_MODE)
    	dbg_print("\t<--- "); dbg_println_size(_STORAGE.RxIndex, &_STORAGE.RxBuffer[AT_RESPONSE_OFFSET]);
#endif
        ret = true;
    }

    /* if buffer overflow but but receive OK mean success! */
    if(strcmp((char*)reply, "OK") == 0){
    	if(_STORAGE.Flags.FlagOK == true){
    		ret = true;
    	}
    }

    return ret;
}

//#########################################################################################################
bool AT_sendNoCheck(uint8_t *send){
    bool ret = true;

    AT_RxClear();
    AT_TxClear();
    AT_ClearAllFlag();

    sprintf((char*)_STORAGE.TxBuffer,"%s\r\n", send);
#if(AT_DEBUG_MODE)
    	dbg_print("\t---> "); dbg_println(send);
#endif

    if(AT_sendCommand((uint8_t*)_STORAGE.TxBuffer)==false){
        ret = false;
    }

    return ret;
}

//#########################################################################################################
bool AT_waitForString(uint8_t *result, uint32_t timeOut, uint8_t CountOfParameter, ...)
{
	if(result == NULL)
	{
		return false;
	}
	if(CountOfParameter == 0)
	{
		return false;
	}

	*result=0;

  	va_list tag;
	va_start (tag,CountOfParameter);
	char *arg[CountOfParameter];
	for(uint8_t i=0; i<CountOfParameter ; i++)
	{
		arg[i] = va_arg (tag, char *);
	}
  	va_end (tag);
	//////////////////////////////////
	for(uint32_t t=0 ; t<timeOut ; t+=50)
	{
		HAL_Delay(50);
		for(uint8_t	mx=0 ; mx<CountOfParameter ; mx++)
		{
			if(strstr((char*)&_STORAGE.RxBuffer[AT_RESPONSE_OFFSET],arg[mx])!=NULL)
			{
				*result = mx+1;
				return true;
			}
		}
	}
	// timeout
	return false;
}

//#########################################################################################################
bool AT_returnString(uint8_t *result,uint8_t WantWhichOne,uint8_t *SplitterChars)
{
	char *str = NULL;

	if(result == NULL){
		return false;
	}

	if(WantWhichOne==0){
		return false;
	}

	str = (char*)_STORAGE.RxBuffer;
	str = strtok (str,(char*)SplitterChars);
	if(str == NULL){
		strcpy((char*)result,"");
		return false;
	}

	while (str != NULL){
		str = strtok (NULL,(char*)SplitterChars);
		if(str != NULL){
			WantWhichOne--;
		}

		if(WantWhichOne==0){
			strcpy((char*)result,str);
			return true;
		}
	}
	strcpy((char*)result,"");
	return false;
}

//#########################################################################################################
// Todo: Please investigate work flow
uint16_t AT_readline(int16_t timeout, bool multiline){
    uint16_t ret = 0;
    int16_t TimeOut = timeout;

	do{
		if(_STORAGE.Flags.FlagCRLF == true){
			_STORAGE.Flags.FlagCRLF = false;

			if (!multiline){
				ret = _STORAGE.RxIndex;
				timeout = 0;
				break;
			}
		}
		HAL_Delay(100);
		TimeOut -= 100;
	}while(TimeOut > 0);

    return ret;
}

//#########################################################################################################
uint16_t AT_readlineOK(int16_t timeout){
    uint16_t len = 0;
    int16_t TimeOut = timeout;

	do{
		if(_STORAGE.Flags.FlagOK == true){
			len = _STORAGE.RxIndex;
			timeout = 0;
			break;
		}
		HAL_Delay(100);
		TimeOut -= 100;
	}while(TimeOut > 0);

    return len;
}

//#########################################################################################################
uint8_t AT_getReply(uint8_t *send, uint16_t timeout)
{
	uint8_t len = 0;

    AT_RxClear();
    AT_TxClear();
    AT_ClearAllFlag();

    sprintf((char*)_STORAGE.TxBuffer,"%s\r\n", send);
#if(AT_DEBUG_MODE)
    	dbg_print("\t---> "); dbg_println(send);
#endif

    if(AT_sendCommand((uint8_t*)_STORAGE.TxBuffer)==false){
        return len;
    }

    len = (uint8_t) AT_readline(timeout, false);
#if(AT_DEBUG_MODE)
	  dbg_print("\t<--- "); dbg_println_size(_STORAGE.RxIndex, &_STORAGE.RxBuffer[AT_RESPONSE_OFFSET]);
#endif

   /* Length of data received */
   return len;
}

//#########################################################################################################
bool AT_parseReply(uint8_t* toreply, uint32_t *v, uint8_t divider, uint8_t index){
  char* pch;
  /* get the pointer to the needed word */
  char *p = strstr((char*)&_STORAGE.RxBuffer[AT_RESPONSE_OFFSET], (char*)toreply);
  if (p == 0){
	  return false;
  }

  p += strlen((char*)toreply);
  for (uint8_t i=0; i<index;i++){
    // increment dividers
    p = strchr(p, divider);
    if (!p){
    	return false;
    }
    p++;
  }

  *v = strtol(p, &pch, 10);

  return true;
}

//#########################################################################################################
bool AT_sendParseReply(uint8_t* tosend, uint8_t* toreply, uint32_t *v, uint8_t divider, uint8_t index){
  AT_getReply(tosend, 500);

  if (! AT_parseReply(toreply, v, divider, index)){
	  return false;
  }
  AT_readlineOK(1000); // until get 'OK' or time out

  return true;
}

//#########################################################################################################
bool AT_expectReplyOK(uint8_t *send, uint16_t timeout){
	bool result = false;
	bool onLoop = true;
	uint32_t TickStart = HAL_GetTick();
	uint32_t TimeOut = (uint32_t)timeout;

    AT_RxClear();
    AT_TxClear();
    AT_ClearAllFlag();

    sprintf((char*)_STORAGE.TxBuffer,"%s\r\n", send);
#if(AT_DEBUG_MODE)
    	dbg_print("\t---> "); dbg_println(send);
#endif
    if(AT_sendCommand((uint8_t*)_STORAGE.TxBuffer)==false){
        return false;
    }

    while(((HAL_GetTick() - TickStart) <= TimeOut) && (onLoop == true)){
    	if(_STORAGE.Flags.FlagOK == true){
    		result = true;
    		onLoop = false;
    	}
    	if(_STORAGE.Flags.FlagError == true){
    		result = false;
    		onLoop = false;
    	}
    }
#if(AT_DEBUG_MODE)
	HAL_Delay(100);
	dbg_print("\t<--- "); dbg_println_size(_STORAGE.RxIndex, &_STORAGE.RxBuffer[AT_RESPONSE_OFFSET]);
#endif
  /* time-out */
  return result;
}

//#########################################################################################################
bool AT_sendCheckReplyByte(uint8_t *send, uint8_t *need, uint16_t timeout)
{
	uint32_t Timeout    = (uint32_t)timeout;
    uint32_t TickStart  = HAL_GetTick();

    AT_RxClear();
    AT_TxClear();
    AT_ClearAllFlag();

    sprintf((char*)_STORAGE.TxBuffer,"%s\r\n", send);
#if(AT_DEBUG_MODE)
    	dbg_print("\t---> "); dbg_println(send);
#endif
    if(AT_sendCommand((uint8_t*)_STORAGE.TxBuffer)==false){
        return false;
    }

    do{
    	if(_STORAGE.Flags.FlagResponse == true){
			if(_STORAGE.RxBuffer[2] == *need){
				// return immediately for exit function
				return true;
			}
			else{
				return false;
			}
		}
    }while((HAL_GetTick() - TickStart) <= Timeout);

    /* Time-out */
    return false;
}


//////////////////////////////////////////////////////////////////////////
// STATIC
//////////////////////////////////////////////////////////////////////////

bool AT_sendCommand(uint8_t *toSend)
{
//	printf((char*)toSend);
	_STORAGE.Flags.FlagEndCmd = true;
	printf("%s",(char*)toSend);
	return true;
}

void AT_catchOK(void)
{
	/* Due to disable echo. So, text receive will be response only */
	switch(_State_OK)
	{
		case 0:	_State_OK = (_STORAGE.ByteRecv == 'O')  ? 1 : 0 ;	break;
		case 1:	_State_OK = (_STORAGE.ByteRecv == 'K')  ? 2 : 0 ;	break;
		case 2:	_State_OK = (_STORAGE.ByteRecv == '\r') ? 3 : 0 ;	break;
		case 3: if(_STORAGE.ByteRecv == '\n') { _STORAGE.Flags.FlagOK = true; }
			_State_OK = 0;
		break;

		default:
			_State_OK = 0;
		break;
	}
}

void AT_catchRDY(void)
{
	/* Due to disable echo. So, text receive will be response only */
	switch(_State_RDY)
	{
		case 0:	_State_RDY = (_STORAGE.ByteRecv == 'R')  ? 1 : 0 ;	break;
		case 1:	_State_RDY = (_STORAGE.ByteRecv == 'D')  ? 2 : 0 ;	break;
		case 2:	_State_RDY = (_STORAGE.ByteRecv == 'Y')  ? 3 : 0 ;	break;
		case 3:	_State_RDY = (_STORAGE.ByteRecv == '\r') ? 4 : 0 ;	break;
		case 4: if(_STORAGE.ByteRecv == '\n') {
			_STORAGE.Flags.FlagRDY = true;
			dbg_println("## READY (UC200T Wakeup) ##");
		}
			_State_RDY = 0;
		break;

		default:
			_State_RDY = 0;
		break;
	}
}

//#########################################################################################################
void AT_catchError(void)
{
	/* Due to disable echo. So, text receive will be response only */
	switch(_State_ERROR)
	{
		case 0:	_State_ERROR = (_STORAGE.ByteRecv == 'E')  ? 1 : 0;	break;
		case 1:	_State_ERROR = (_STORAGE.ByteRecv == 'R')  ? 2 : 0;	break;
		case 2:	_State_ERROR = (_STORAGE.ByteRecv == 'R')  ? 3 : 0;	break;
		case 3:	_State_ERROR = (_STORAGE.ByteRecv == 'O')  ? 4 : 0;	break;
		case 4:	_State_ERROR = (_STORAGE.ByteRecv == 'R')  ? 5 : 0;	break;
		case 5:	_State_ERROR = (_STORAGE.ByteRecv == '\r') ? 6 : 0;	break;
		case 6:	if(_STORAGE.ByteRecv == '\n'){ _STORAGE.Flags.FlagError = true; }
			_State_ERROR = 0;

		break;

		default:
			_State_ERROR = 0;
		break;
	}
}

//#########################################################################################################
void AT_TxClear(void){
   memset((char*)_STORAGE.TxBuffer, AT_SET_ZERO, AT_CORE_BUFFER_SIZE);
}

//#########################################################################################################
void AT_RxClear(void){
   _STORAGE.RxIndex = AT_SET_ZERO;
   memset((char*)_STORAGE.RxBuffer, AT_SET_ZERO, AT_CORE_BUFFER_SIZE);
}

//#########################################################################################################
void AT_ClearAllFlag(void){
	_STORAGE.Flags.AllFlags = AT_SET_ZERO;
}

//#########################################################################################################
