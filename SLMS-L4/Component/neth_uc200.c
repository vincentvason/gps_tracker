/*
 * neth_UC200.cc
 *
 *  Created on: Mar 17, 2020
 *      Author: Saifa-NB
 */

#include "neth_uc200.h"
#include "neth_ATparser.h"

/* Global variable */
extern volatile AT_t _STORAGE;
static UC200_Conf _CONFIG = {0};
static UART_HandleTypeDef *UartInst = NULL;


/* Lookup table */
const uint8_t APN_List[5][3][32] = {
		{"internet"          , "true", "true"},
		{"internet"          , ""    , ""},
		{"www.dtac.co.th"    , ""    , ""},
		{""                  , ""    , ""},
		{""                  , ""    , ""}
};


bool APNlookupfromMccMnc(uint16_t mccmnc, uint8_t *apn, uint8_t *usr, uint8_t *pwd){
	bool lookupPassed = false;

	switch (mccmnc) {
		case MMCMNC_TrueMove:{
			memcpy(apn, APN_List[0][0], strlen((char*)APN_List[0][0]));
			memcpy(usr, APN_List[0][1], strlen((char*)APN_List[0][1]));
			memcpy(pwd, APN_List[0][2], strlen((char*)APN_List[0][2]));
			lookupPassed = true;
			break;
		}

		case MMCMNC_AIS:{
			memcpy(apn, APN_List[1][0], strlen((char*)APN_List[1][0]));
			memcpy(usr, APN_List[1][1], strlen((char*)APN_List[1][1]));
			memcpy(pwd, APN_List[1][2], strlen((char*)APN_List[1][2]));
			lookupPassed = true;
			break;
		}

		case MMCMNC_DTAC:{
			memcpy(apn, APN_List[2][0], strlen((char*)APN_List[2][0]));
			memcpy(usr, APN_List[2][1], strlen((char*)APN_List[2][1]));
			memcpy(pwd, APN_List[2][2], strlen((char*)APN_List[2][2]));
			lookupPassed = true;
			break;
		}

		case MMCMNC_TrueByCAT:{
			memcpy(apn, APN_List[1][0], strlen((char*)APN_List[1][0]));
			memcpy(usr, APN_List[1][1], strlen((char*)APN_List[1][1]));
			memcpy(pwd, APN_List[1][2], strlen((char*)APN_List[1][2]));
			lookupPassed = true;
			break;
		}

		case MMCMNC_TOT:{
			memcpy(apn, APN_List[1][0], strlen((char*)APN_List[1][0]));
			memcpy(usr, APN_List[1][1], strlen((char*)APN_List[1][1]));
			memcpy(pwd, APN_List[1][2], strlen((char*)APN_List[1][2]));
			lookupPassed = true;
			break;
		}
		default:
			lookupPassed = false;
			break;
	}

	return lookupPassed;
}

/* Please place this function on file stm32xxxx_it under your communication UART */
void (*UC200_RxCallback)(void);


#define UC200_HW_WakeUp()      HAL_GPIO_WritePin(STM_DTR_GSM_GPIO_Port, STM_DTR_GSM_Pin, GPIO_PIN_RESET)
#define UC200_HW_Sleep()       HAL_GPIO_WritePin(STM_DTR_GSM_GPIO_Port, STM_DTR_GSM_Pin, GPIO_PIN_SET)
#define UC200_HW_RST_ON()      HAL_GPIO_WritePin(GSM_RST_GPIO_Port, GSM_RST_Pin, GPIO_PIN_SET)
#define UC200_HW_RST_OFF()     HAL_GPIO_WritePin(GSM_RST_GPIO_Port, GSM_RST_Pin, GPIO_PIN_RESET)
#define UC200_HW_PWRKEY_HIGH() HAL_GPIO_WritePin(GSM_PWR_KEY_GPIO_Port, GSM_PWR_KEY_Pin, GPIO_PIN_RESET)
#define UC200_HW_PWRKEY_LOW()  HAL_GPIO_WritePin(GSM_PWR_KEY_GPIO_Port, GSM_PWR_KEY_Pin, GPIO_PIN_SET)



void UC200_genPowerOnPulse(void){
	/* Power Key generating until module ready */
	UC200_HW_PWRKEY_LOW();
	HAL_Delay(22);
	UC200_HW_RST_OFF();
	HAL_Delay(688);
	UC200_HW_PWRKEY_HIGH();
}

void UC200_genPowerOffPulse(void){
	/* Power Key generating until module ready */
	UC200_HW_PWRKEY_LOW();
	HAL_Delay(700); /* Range:  t >= 650ms */
	UC200_HW_PWRKEY_HIGH();
}

bool UC200_init(UART_HandleTypeDef *huart)
{
	/* Buffer and interrupt initialize */
	UC200_RxCallback = AT_RxCallback;

	UartInst = huart;

	if(UartInst == NULL){
		return false;
	}

#if(AT_DEBUG_MODE)
	dbg_print("## Initialize AT parser ... ");
#endif
	AT_init(UartInst);
#if(AT_DEBUG_MODE)
	dbg_println("DONE");
#endif
	/* Hardware Initialize */
#if(AT_DEBUG_MODE)
	dbg_print("## Generating power key signal ... ");
#endif

	/* Recommended Restart process: Quectel_UC20_AT_Commands_Application_Note */
	UC200_HW_WakeUp();       // Hardware Wake-up
	UC200_HW_RST_ON();       // Hardware Reset use

	if(HAL_GPIO_ReadPin(STM_CTS_GSM_GPIO_Port, STM_CTS_GSM_Pin)){
		/* 1. Power off */
		UC200_genPowerOffPulse();
		/* 2. Delay at least 3000ms */
		HAL_Delay(3000);
		/* 3. Power on */
		UC200_genPowerOnPulse();
	}

#if(AT_DEBUG_MODE)
	dbg_println("DONE");
#endif

	/* Try to attempt AT, disable echo then wait for SARA can go to stable */
#if(AT_DEBUG_MODE)
	dbg_print("Sending AT commands ... ");
#endif
	while(UC200_syncModule(10000U) == false){
		/* After 10sec timeout try to re-generate power on pulse */
		UC200_genPowerOnPulse();
	}
#if(AT_DEBUG_MODE)
	dbg_println("DONE");
#endif
	UC200_echoCommand(DEACTIVE);
	UC200_echoErrorMessage(DEACTIVE);
#if(AT_DEBUG_MODE)
	dbg_print("Waiting 3sec for module stable ... ");
#endif
	HAL_Delay(3000);
#if(AT_DEBUG_MODE)
	dbg_println("Ready!");
#endif

	return true;
}



bool UC200_setBaudrate(uint32_t baud)
{
	uint8_t cmd[64] = {0};

	memset((char*)cmd, 0, 64);	// Make sure buffer is clear before write
	sprintf((char*)cmd, "AT+IPR=%lu", baud);
	return (AT_expectReplyOK(cmd, AT_TIMEOUT_1000MS));
}


bool UC200_setFunctionality(uint8_t option, uint8_t reset)
{
	 uint8_t cmd[64] = {0};

	 memset((char*)cmd, 0, 64);	// Make sure buffer is clear before write
	 sprintf((char*)cmd, "AT+CFUN=%u,%u" ,option, reset);
	 return (AT_expectReplyOK(cmd,  AT_TIMEOUT_1000MS));
}

bool UC200_syncModule(uint32_t timeOut){
	bool    Synchronized = false;
	bool    SyncResult   = false;
	uint8_t SyncCounter  = 0;
	uint32_t TickStart = HAL_GetTick();

	do{
		Synchronized = AT_expectReplyOK((uint8_t*)"AT", AT_TIMEOUT_1000MS);
		if(Synchronized == true){
			SyncCounter += 1;
			HAL_Delay(10);
		}
		else{
			HAL_Delay(AT_TIMEOUT_1000MS);
		}
	}while((SyncCounter <= 3) && ((HAL_GetTick() - TickStart) < timeOut));

	if(SyncCounter > 3){
		SyncResult = true;
	}

	return SyncResult;
}


bool UC200_echoCommand(uint8_t mode)
{
	bool success = false;

	switch(mode)
	{
	case DEACTIVE:
		success = AT_expectReplyOK((uint8_t*)"ATE0", AT_TIMEOUT_1000MS);
		break;

	case ACTIVE:
		success = AT_expectReplyOK((uint8_t*)"ATE1", AT_TIMEOUT_1000MS);
		break;

	default:
		__NOP();
		break;
	}

	return success;
}

bool UC200_echoErrorMessage(uint8_t mode)
{
	bool success = false;

	switch(mode)
	{
	case DEACTIVE:
		success = AT_expectReplyOK((uint8_t*)"AT+CMEE=0", AT_TIMEOUT_1000MS);
		break;

	case ACTIVE:
		success = AT_expectReplyOK((uint8_t*)"AT+CMEE=2", AT_TIMEOUT_1000MS);
		break;

	default:
		__NOP();
		break;
	}

	return success;
}

/* Network */
bool UC200_setFormatOperator(uint8_t format){
    uint8_t cmd[64] = {0};

    memset((char*)cmd, 0, 64);	// Make sure buffer is clear before write
    sprintf((char*)cmd, "AT+COPS=3,%u", format);
    return (AT_expectReplyOK(cmd, AT_TIMEOUT_1000MS));
}

bool UC200_deRegistered(uint8_t mode)
{
    uint8_t cmd[64] = {0};
    uint32_t TickStart = HAL_GetTick();
    uint32_t TimeOut = AT_TIMEOUT_20000MS;

    memset((char*)cmd, 0, 64);	// Make sure buffer is clear before write
    sprintf((char*)cmd, "AT+COPS=%d", (int)(mode << 1));

    // Possible to return "ERROR" if no SIM card
    AT_sendNoCheck(cmd);
    while((HAL_GetTick() - TickStart) <= TimeOut)
    {
    	if(_STORAGE.Flags.FlagOK == true)
    	{
    		return (true);
    	}
    	if(_STORAGE.Flags.FlagError == true)
    	{
    		return (false);
    	}
    }

    // time-out
    return false;
}

bool UC200_setPreferredScan(uint8_t mode, uint8_t apply)
{
	uint8_t cmd[64] = {0};

	memset((char*)cmd, 0, 64);	// Make sure buffer is clear before write
	sprintf((char*)cmd, "AT+QCFG=\"nwscanmode\",%u,%u", mode, apply);
	return (AT_expectReplyOK(cmd, AT_TIMEOUT_10000MS));
}

bool UC200_setPreferredPriority(uint8_t mode, uint8_t apply)
{
	uint8_t cmd[64] = {0};

	memset((char*)cmd, 0, 64);	// Make sure buffer is clear before write
	sprintf((char*)cmd, "AT+QCFG=\"nwscanseq\",%u,%u", mode, apply);
	return (AT_expectReplyOK(cmd, AT_TIMEOUT_10000MS));
}


uint8_t UC200_getNetworkStatus(uint8_t mode)
{
	uint32_t status;

	if(mode == PREFERRED_2G_3G){
		if (! AT_sendParseReply((uint8_t*)"AT+CREG?", (uint8_t*)"+CREG: ", &status, ',', 1))
		{
			return OPR_NOT_WORKING;
		}
	}
	if(mode == PREFERRED_3G){
		if (! AT_sendParseReply((uint8_t*)"AT+CGREG?", (uint8_t*)"+CGREG: ", &status, ',', 1))
		{
			return OPR_NOT_WORKING;
		}
	}

	return (uint8_t)(status);
}

uint8_t UC200_getCCID(uint8_t *buffer)
{
	uint16_t TextLen;
	uint8_t  dataLen;

	 // ISO/IEC 7812. According to E.118, the number can be up to 22 digits long.
	 if(true == AT_expectReplyOK((uint8_t*)"AT+QCCID", AT_TIMEOUT_1000MS))
	 {
		 /* Get size of data received, but not over 22 digit */
		 TextLen = (_STORAGE.RxIndex >= 22U) ? 22U : (_STORAGE.RxIndex - AT_RESPONSE_OFFSET);
		 dataLen = (uint8_t)TextLen;

		 memcpy(buffer, (uint8_t*)&_STORAGE.RxBuffer[10], dataLen);
		 buffer[dataLen] = 0;
	 }
	 else
	 {
		 sprintf((char*)buffer, "ERROR");
		 dataLen = 0;
	 }

	 return dataLen;
}

uint8_t UC200_getIMEI(uint8_t *buffer)
{
	uint16_t TextLen;
	uint8_t  dataLen;

	 // Up to 15 character long
	 if(true == AT_expectReplyOK((uint8_t*)"AT+GSN", AT_TIMEOUT_1000MS))
	 {
		 TextLen = (_STORAGE.RxIndex >= 15U) ? 15U : (_STORAGE.RxIndex - AT_RESPONSE_OFFSET);
		 dataLen = (uint8_t)TextLen;

		 memcpy(buffer, (uint8_t*)&_STORAGE.RxBuffer[AT_RESPONSE_OFFSET], dataLen);
		 buffer[dataLen] = 0;
	 }
	 else
	 {
		 sprintf((char*)buffer, "ERROR");
		 dataLen = 0;
	 }

	 return dataLen;
}


uint8_t UC200_getCSQ(void)
{
	uint16_t status;

	if (! AT_sendParseReply((uint8_t*)"AT+CSQ", (uint8_t*)"+CSQ: ", &status, ',', 0) )
	{
	   return 99;
	}
	return status;
}


/* Internet */
bool UC200_GPRSsetParam(uint8_t* APN, uint8_t* User, uint8_t* Pass)
{
	//PDP context setting @CID1
	uint8_t cmd[128] = {0};
	uint16_t apnLen = (uint16_t) strlen((char*)APN);
	uint16_t usrLen = (uint16_t) strlen((char*)User);
	uint16_t pwdLen = (uint16_t) strlen((char*)Pass);

	/* Copy APN parameter to private configuration parameter */
	if((0 != apnLen) && (32 >= apnLen))
	{
		memcpy((char*)_CONFIG.apnName, (char*)APN, apnLen);
	}
	if((0 != usrLen) && (32 >= usrLen))
	{
		memcpy((char*)_CONFIG.apnUser, (char*)User, usrLen);
	}
	if((0 != pwdLen) && (32 >= pwdLen))
	{
		memcpy((char*)_CONFIG.apnPass, (char*)Pass, pwdLen);
	}

	memset((char*)cmd, 0, 128);
	sprintf((char*)cmd, "AT+CGDCONT=1,\"IP\",\"%s\"", APN);
	return (AT_expectReplyOK((uint8_t*)cmd, AT_TIMEOUT_10000MS));
}

uint16_t UC200_getMccMnc(void)
{
	uint8_t buffer[8] = {0};
	uint16_t mccmnc = 0;

	/* Module response correctly? */
	if (!AT_expectReplyOK((uint8_t*)"AT+COPS?", AT_TIMEOUT_1000MS))
	{
		return 0;
	}

	/* Return MCCMNC message to buffer */
	if(!AT_returnString(buffer, 1, (uint8_t*)"\""))
	{
		return 0;
	}

	/* Cast to integer */
	mccmnc = (uint16_t)atoi((char*)buffer);

	return mccmnc;
}

bool UC200_GPRSgetParamFromList(void)
{
	//PDP context setting @CID1
	uint8_t cmd[128] = {0};
	uint16_t mccmnc = {0};
	bool Status = false;

	/* Set format of COPS to MCCMNC first */
	UC200_setFormatOperator(OPR_FORMAT_MCCMNC);

	/* clean configuration parameter */
	memset((char*)_CONFIG.apnName, 0, 32);
	memset((char*)_CONFIG.apnUser, 0, 32);
	memset((char*)_CONFIG.apnPass, 0, 32);

	/* Get MCCMNC */
	mccmnc = UC200_getMccMnc();

	/* Lookup table */
	Status = APNlookupfromMccMnc(mccmnc, _CONFIG.apnName, _CONFIG.apnUser, _CONFIG.apnPass);

	if(Status == true){
		memset((char*)cmd, 0, 128);
		sprintf((char*)cmd, "AT+CGDCONT=1,\"IP\",\"%s\"", _CONFIG.apnName);
		Status = AT_expectReplyOK((uint8_t*)cmd, AT_TIMEOUT_10000MS);
	}

	return Status;
}

bool UC200_GPRSenable(uint8_t mode)
{
	 uint8_t cmd[256] = {0};

	 if (mode == ACTIVE)
	 {
	  //  GPRS attachment -> attached possible both OK and ERROR
	  if (! AT_expectReplyOK((uint8_t*)"AT+CGATT=1", AT_TIMEOUT_10000MS))
	  {
	    return false;
	  }

	  HAL_Delay(200); // This seems to help the next line run the first time

	  // Packet barrier for TCPIP
	  if (strlen((char*)_CONFIG.apnName) > 0)
	  {
			// Configured APN value @Profile0
			memset((char*)cmd, 0, 256);	// Make sure buffer is clear before write
			sprintf((char*)cmd, "AT+QICSGP=1,1,\"%s\"", _CONFIG.apnName);

			// set username/password
			if (strlen((char*)_CONFIG.apnUser) > 0)
			{
			  strcat((char*)cmd, ",\"");
			  strcat((char*)cmd, (char*)_CONFIG.apnUser);
			}
			if (strlen((char*)_CONFIG.apnPass) > 0)
			{
				strcat((char*)cmd, "\",\"");
				strcat((char*)cmd, (char*)_CONFIG.apnPass);
				strcat((char*)cmd, "\"");
			}

			// Configured APN user, password @Profile0
			if (! AT_expectReplyOK(cmd, AT_TIMEOUT_10000MS))
			{
				return false;
			}
        }

	    // PDP context activate @CID1
	    if(! AT_expectReplyOK((uint8_t*)"AT+CGACT=1,1", AT_TIMEOUT_30000MS))
	    {
	      return false;
	    }

	 }
	 else
	 {
	    // Packet switched data reset
		 AT_expectReplyOK((uint8_t*)"AT+CGACT=0,1", AT_TIMEOUT_20000MS);

		// TCPIP barrier close
		 if (! AT_expectReplyOK((uint8_t*)"AT+QIACT=1", AT_TIMEOUT_10000MS))
		  {
			return false;
		  }

	   //  GPRS attachment -> de-attached
	  if (! AT_expectReplyOK((uint8_t*)"AT+CGATT=0", AT_TIMEOUT_10000MS))
	  {
	    return false;
	  }
	 }

	 return true;
}

bool UC200_GPRScheckIP(void)
{
	if(! AT_expectReplyOK((uint8_t*)"AT+CGPADDR=1", AT_TIMEOUT_1000MS))
	{
		return false;
	}
	return true;
}


bool UC200_RTCenableTimeSync(uint8_t mode)
{
	uint8_t cmd[64] = {0};
	uint16_t status;

	// Get current configuration
	AT_sendParseReply((uint8_t*)"AT+CTZU?", (uint8_t*)"+CTZU:", &status, ' ', 1);

	// If configuration not same with current input will re-config
	if(status != (uint16_t)(mode))
	{
		// Set automatic time zone update
		memset((char*)cmd, 0, 64);	// Make sure buffer is clear before write
		sprintf((char*)cmd, "AT+CTZU=%d", (int)mode);
		if(! AT_expectReplyOK(cmd, AT_TIMEOUT_1000MS))
		{
		  return false;
		}

		// Save to NVM
		if(! AT_expectReplyOK((uint8_t*)"AT&W", AT_TIMEOUT_1000MS))
		{
		  return false;
		}
		// Reboot for apply profile.
#if(AT_DEBUG_MODE)
		dbg_println("Synchronize time updated ... resetting system for take effect.");
#endif
		HAL_NVIC_SystemReset();
	}

	return true;
}

bool UC200_RTCread(uint8_t* buffer)
{
	/* Module response correctly? */
	if (!AT_expectReplyOK((uint8_t*)"AT+CCLK?", AT_TIMEOUT_1000MS))
	{
		return false;
	}

	/* Return time message */
	if(!AT_returnString(buffer, 1, (uint8_t*)"\""))
	{
		return false;
	}
	return true;
}

bool UC200_ConvertEpochTime(char* str, uint64_t* epoch_time){

	//Array of Calendar (y,m,d,hh,mm,ss)
	uint64_t epoch = 0;
	char* pch;
	uint16_t ymd[6];

	pch = strtok (str,","); //yy/mm/dd
	ymd[0] = (pch[0] - '0')*10 + (pch[1] - '0'); //Get Year
	ymd[0] = ymd[0] + 2000;
	if(pch[2] != '/') return false;
	ymd[1] = (pch[3] - '0')*10 + (pch[4] - '0'); //Get Month
	if(pch[5] != '/') return false;
	ymd[2] = (pch[6] - '0')*10 + (pch[7] - '0'); //Get Day
	pch = strtok (NULL,"+"); //Day
	ymd[3] = (pch[0] - '0')*10 + (pch[1] - '0'); //Get Hours
	if(pch[2] != ':') return false;
	ymd[4] = (pch[3] - '0')*10 + (pch[4] - '0'); //Get Minutes
	if(pch[5] != ':') return false;
	ymd[5] = (pch[6] - '0')*10 + (pch[7] - '0'); //Get Seconds

		    for(int i = 0; i < ymd[0]-1970; i++)
		    {
		        if((1970+i)%4 == 0)
		        {
		            epoch += (86400*366);
		        }
		        else
		        {
		            epoch += (86400*365);
		        }
		    }

		    switch (ymd[1])
		    {
		        case 12: epoch += (86400*30);
		        case 11: epoch += (86400*31);
		        case 10: epoch += (86400*30);
		        case  9: epoch += (86400*31);
		        case  8: epoch += (86400*31);
		        case  7: epoch += (86400*30);
		        case  6: epoch += (86400*31);
		        case  5: epoch += (86400*30);
		        case  4: epoch += (86400*31);
		        case  3:
		            if(ymd[0]%4 == 0)
		            {
		                epoch += (86400*29);
		            }
		            else
		            {
		                epoch += (86400*28);
		            }
		        case  2: epoch += (86400*31);
		        case  1: break;
		        default: break;
		    }

		    epoch += ((ymd[2] - 1)*86400);
		    epoch += (ymd[3]*3600);
		    epoch += (ymd[4]*60);
		    epoch += (ymd[5]*1);

		    *epoch_time = epoch;
		    //printf("%lld",epoch);

		  return true;
}

bool UC200_TcpipOpen(uint8_t* server, int port)
{
	  uint8_t cmd[128] ={0};
//	  uint16_t timeout = 0;

	  sprintf((char*)cmd, "AT+QIOPEN=1,0,\"TCP\",\"%s\",%d,0,0", (char*)server, port);
	  return(AT_sendCheckReply(cmd, (uint8_t*)"QIOPEN", AT_TIMEOUT_10000MS));
}

bool UC200_TcpipClose(void)
{
	return(AT_expectReplyOK((uint8_t*)"AT+QICLOSE=0", AT_TIMEOUT_1000MS));
}

bool UC200_TcpipSend(uint8_t* msg, int len)
{
	  uint8_t cmd[128] = {0};
	  sprintf((char*)cmd, "AT+QISEND=0,%d", len);
	  AT_sendNoCheck(cmd);

	  if(_STORAGE.RxBuffer[2] != '>')
	  {
		  return false;
	  }

	  return(AT_expectReplyOK(msg, AT_TIMEOUT_1000MS));
}

bool UC200_TcpipReceive(uint16_t len)
{
	  uint8_t cmd[128] = {0};
	  sprintf((char*)cmd, "AT+QIRD=0,%d", len);
	  return(AT_sendCheckReply(cmd, (uint8_t*)"+QIRD", AT_TIMEOUT_5000MS));
}


void UC200_MqttConnectMessage (uint8_t*msg, const uint8_t* id, const uint8_t* user, const uint8_t* pass)
{
	   int r_ind = 0;

	    //FIXED HEADER
	    //Control
	    msg[r_ind++] = (1 << 4);
	    //Length
	    r_ind++; //Will get actual length later

	    //VARIABLE HEADER
	    //Length
	    msg[r_ind++] = 0x00;
	    msg[r_ind++] = 0x04;
	    //Protocol Name
	    msg[r_ind++] = 'M';
	    msg[r_ind++] = 'Q';
	    msg[r_ind++] = 'T';
	    msg[r_ind++] = 'T';
	    //Protocol Version
	    msg[r_ind++] = 4;
	    //Protocol Flag
	    msg[r_ind] = 0x02; //QoS0
	    if(user != NULL) msg[r_ind] = msg[r_ind] | 0x80;
	    if(user != NULL) msg[r_ind] = msg[r_ind] | 0x40;
	    r_ind++;
	    //Keep Alive
	    msg[r_ind++] = 0;
	    msg[r_ind++] = 120;

	    //PAYLOAD - ID
	    //Length
	    msg[r_ind++] = 0x00; //Fix MSB not more than 127 char.
	    msg[r_ind++] = strlen((char*)id);
	    //ID
	    for(int i = 0;i < (int)strlen((char*)id); i++){
	        msg[r_ind++] = id[i];
	    }

	    //PAYLOAD - USER
	    if(user != NULL){
	      //Length
	      msg[r_ind++] = 0x00; //Fix MSB not more than 127 char.
	      msg[r_ind++] = strlen((char*)user);
	      //ID
	      for(int i = 0;i < (int)strlen((char*)user); i++){
	          msg[r_ind++] = user[i];
	      }
	    }

	    //PAYLOAD - PASS
	    if(pass != NULL){
	      //Length
	      msg[r_ind++] = 0x00; //Fix MSB not more than 127 char.
	      msg[r_ind++] = strlen((char*)pass);
	      //ID
	      for(int i = 0;i < (int)strlen((char*)pass); i++){
	          msg[r_ind++] = pass[i];
	      }
	    }

	    //Summarize remaining length
	    msg[1] = r_ind-2;
}

void UC200_MqttPublishMessage(uint8_t* msg, const uint8_t* topic, const uint8_t* data)
{
	int r_ind = 0;

	//FIXED HEADER
	//Control
	msg[r_ind++] = (3 << 4); //QoS0
	//Length
	r_ind++; //Will get actual length later
	if((strlen((char*)topic) + strlen((char*)data)) >= 123){
		r_ind++; //Will get actual length later
				 //get another length byte if contain more than 123 bytes.
	}
	//VARIABLE HEADER
	//Topic Length
	msg[r_ind++] = 0x00;
	msg[r_ind++] = strlen((char*)topic);
	for(int i = 0;i < (int)strlen((char*)topic); i++){
		msg[r_ind++] = topic[i];
	}
	//Topic Identifier
//	msg[r_ind++] = 0; //it is for QoS1 or QoS2
//	msg[r_ind++] = 10;
	//PAYLOAD
	//Message
	for(int i = 0;i < (int)strlen((char*)data); i++){
	    msg[r_ind++] = data[i];
	}

	//sprintf(uc20_demsg,"\033[0;36mSummarize Package = %d\033[0m\r\n",r_ind);
	//dbg_print(uc20_demsg);
	//Summarize remaining length
	if(r_ind < 128){
		r_ind = r_ind - 2;
		msg[1] = r_ind;
	}
	else if(r_ind >= 128 && r_ind < 16384){
		r_ind = r_ind - 3;
		msg[1] = r_ind | 0x80;
		msg[2] = r_ind / 0x80;
	}
	else{
		r_ind = r_ind - 3;
		msg[1] = 0xFF;
		msg[2] = 0x7F;
	}
}

void UC200_MQTTSubscribeMessage(uint8_t *msg, const uint8_t *topic, uint8_t QoS)
{
	int r_ind = 0;

	// Fixed Header
	msg[r_ind++] = 0x82;

	//Length
	r_ind++; //Will get actual length later

	// Variable header
	msg[r_ind++] = 0;
	msg[r_ind++] = 10;

	// Packet payload
	// Payload: topic
	//Topic Length
	msg[r_ind++] = 0x00;
	msg[r_ind++] = strlen((char*)topic);
	for(int i = 0;i < (int)strlen((char*)topic); i++){
		msg[r_ind++] = topic[i];
	}

	// Payload: QoS
	msg[r_ind++] = QoS;

	msg[1] = r_ind - 2;
}


bool UC200T_MQTTsubscribe(const uint8_t* topic, int QoS)
{

	uint8_t mqtt_message[127];
	UC200_MQTTSubscribeMessage(mqtt_message, topic, QoS);

	int len = mqtt_message[1]+2;
	uint8_t cmd[128] = {0};
	sprintf((char*)cmd, "AT+QISEND=0,%d", len);

	if(!AT_sendCheckReplyByte(cmd, (uint8_t*)">", AT_TIMEOUT_2000MS)){
		return false;
	}

	if (!UC200_MqttSendPacket(mqtt_message, len)){
		return false;
	}

	return true;
}

bool UC200_MQTTgetSubscribeMessage(uint8_t *topic, uint8_t *buffer, uint8_t len)
{
	uint8_t cmd[128] = {0};
	int position[10] = {0};
	int i = 0, j = 0;
	int topicLen = strlen((char*)topic);
	size_t copySize = 0;
	uint16_t k = 0;

    sprintf((char*)cmd, "AT+QIRD=0,%u", len);

	if(!AT_expectReplyOK(cmd, AT_TIMEOUT_5000MS))
	{
		return false;
	}

	/* *********************************************************************
	 * Scan for topic if found, cannot strcmp directly because MQTT message
	 * can have white character.
	 * *********************************************************************/
	while(k < _STORAGE.RxIndex){
		if(topic[0] == _STORAGE.RxBuffer[k]){
			position[i] = k;
			i++;

			/* ******************************************************
			 * Expectation is topic will come only 1 topic per time
			 * So, position will not more than 10 with junk message
			 * ******************************************************/
			if(i > 9)
			{
				i = 9;
			}
		}
		k++;
	}

	/* Now have index for strcmp */
	for(j=0;j<i;j++)
	{
		if(0 == strncmp((char*)topic, (char*)&_STORAGE.RxBuffer[position[j]], topicLen))
		{
			/* magic number (8) from \r\n\r\nOK\r\n of UC200 response */
			copySize = (size_t)(_STORAGE.RxIndex - (uint16_t)(position[j] + topicLen + 8));
			memcpy((char*)buffer, (char*)&_STORAGE.RxBuffer[position[j]+topicLen], copySize);
			return true;
		}
	}

	/* No message detected from desire topic*/
	return false;
}

bool UC200_MqttSendPacket(uint8_t *packet, int len)
{
	AT_RxClear();
	char EOP = 26;     // End Of Packet

#if(AT_DEBUG_MODE)
	char AsciiToHex[4];
	dbg_print("\t---> ");
#endif

	for (int j = 0; j < len; j++)
	{
		printf("%c",packet[j]);
#if(AT_DEBUG_MODE)
		sprintf(AsciiToHex,"%02d,",packet[j]);
		dbg_print(AsciiToHex);
#endif
	}
#if(AT_DEBUG_MODE)
	DBG_UART.Instance->TDR = EOP;
#endif

	printf("%c",EOP);

#if(AT_DEBUG_MODE)
   dbg_println("");
   dbg_print ("\t<--- "); dbg_println_size(_STORAGE.RxIndex, (char*)&_STORAGE.RxBuffer[2]);
#endif

   return AT_readlineOK(AT_TIMEOUT_5000MS);
}


bool UC200_MQTTConnect(const uint8_t *protocol, const uint8_t *clientID, const uint8_t *username, const uint8_t *password) {
	char mqtt_message[2048] = {0};
		char cmd[128] = {0};
		int len;
		UC200_MqttConnectMessage(mqtt_message,clientID,username,password);
//		memcpy(mqtt_demsg,mqtt_message,255);
		if((mqtt_message[1] & 0x80) == 0x00){
			len = mqtt_message[1]+2;
		}
		else if((mqtt_message[2] & 0x80) == 0x00){
			len = (mqtt_message[1] & 0x7f)+(mqtt_message[2] << 7)+3;
		}
		else{
			len = 0x7fff;
		}

		sprintf(cmd,"AT+QISEND=0,%d",len);
	AT_sendNoCheck(cmd);
	AT_readline(AT_TIMEOUT_5000MS, false);
  dbg_print("\t<--- ");
  dbg_print((char*)&_STORAGE.RxBuffer[2]);
  if (_STORAGE.RxBuffer[2] != '>')
    return false;

  if (! UC200_MqttSendPacket((uint8_t*)mqtt_message, len))
  {
	  return false;
  }
  return true;
}

uint16_t UC200_TcpipReceiveAvailable(void)
{
   uint16_t dataLen = 0;

   /* Check what is exactly size need to read */
	AT_sendParseReply((uint8_t*)"AT+QIRD=0,0", (uint8_t*)"+QIRD: ", &dataLen, ',', 2);

	return dataLen;
}

bool UC200_MQTTConnectCheck(uint8_t *clientID, uint8_t *username, uint8_t *password){
	char msg[64];
	char asciitohex[4];
	if(UC200_MQTTConnect("MQTT", clientID, username, password))
	{
		HAL_Delay(2000);
	    if(UC200_TcpipReceive(4)){
	    	dbg_print("\033[0;32mMQTT login success\033[0m\r\n\r\n");
	    	for(int i = 0;i < 15;i++){
	    		sprintf(asciitohex, "%02d,", _STORAGE.RxBuffer[i]);
	    		dbg_print(asciitohex);
	    	}
	    	if(_STORAGE.RxBuffer[15] == 0){
	    		return true;
	        }
	    	else{
	    		sprintf(msg,"\033[0;31mMQTT connect failed(result=%d)\033[0m\r\n\r\n",_STORAGE.RxBuffer[15]);
	    		dbg_print(msg);
	    		return false;
	    	}
	    }
	    else{
	    	dbg_print("\033[0;31mMQTT connect failed(receive)\033[0m\r\n\r\n");
	    	return false;
	    }
	}
	else{
		dbg_print("\033[0;31mMQTT connect failed(conn)\033[0m\r\n\r\n");
		return false;
	}
	return 0;
}

bool UC200_MQTTPingCheck(void)
{
	if(UC200_MQTTPing())
	{
	    HAL_Delay(1000);
	    if(UC200_TcpipReceiveAvailable())
	    {
#if(AT_DEBUG_MODE)
	    	dbg_println("MQTT login success\r\n\r\n");
#endif
	    	if(_STORAGE.RxBuffer[9] == '2')
	    	{
	    		return true;
	        }
	    	else
	    	{
	    		return false;
	    	}
	    }
	    else
	    {
#if(AT_DEBUG_MODE)
	    	dbg_println("\033[0;31mMQTT ping failed(2)\033[0m\r\n\r\n");
#endif
	    	return false;
	    }
	}
	else
	{
#if(AT_DEBUG_MODE)
		dbg_println("\033[0;31mMQTT ping failed(1)\033[0m\r\n\r\n");
#endif
		return false;
	}
	return 0;
}

//uint8_t pub_check;
bool UC200_MQTTPublish(const uint8_t* topic, const uint8_t* message)
{
	uint8_t mqtt_message[2048] = {0};
	uint8_t cmd[128] = {0};
	uint8_t check;
	int len;

	UC200_MqttPublishMessage(mqtt_message, topic, message);
	if((mqtt_message[1] & 0x80) == 0x00)
	{
		len = mqtt_message[1] + 2;
	}
	else if((mqtt_message[2] & 0x80) == 0x00)
	{
		len = (mqtt_message[1] & 0x7f)+(mqtt_message[2] << 7)+3;
	}
	else
	{
		len = 0x7FFF;
	}

	AT_RxClear();
	sprintf((char*)cmd,"AT+QISEND=0,%d",len);
#if(AT_DEBUG_MODE)
	dbg_print("\t<--- "); dbg_println_size(_STORAGE.RxIndex, &_STORAGE.RxBuffer[2]);
#endif
	check = AT_sendCheckReply((uint8_t*)cmd,(uint8_t*)">" ,AT_TIMEOUT_1000MS);
	HAL_Delay(1);
	if (strstr(&_STORAGE.RxBuffer[2],">") == NULL)
	{
		return false;
	}

	check = UC200_MqttSendPacket(mqtt_message, len);
	HAL_Delay(1);

	return (check);
}

bool UC200_Filelist(void)
{
	return AT_expectReplyOK((uint8_t*)"AT+QFLST=\"*\"", AT_TIMEOUT_10000MS);
}

bool UC200_FileNew(uint8_t *FileName, uint8_t *data)
{
	uint8_t cmd[128] = {0};
	uint8_t temp[32] = {0};
	uint8_t handleID[32] = {0};
	size_t  handleSize = 0;
	int     dataSize = 0;

	sprintf((char*)cmd, "AT+QFOPEN=\"%s\",1", (char*)FileName);
	if (!AT_expectReplyOK(cmd, AT_TIMEOUT_5000MS))
	{
		return false;
	}
	AT_returnString(temp, 1, (uint8_t*)":");
	/* magic number 9: white space at head and response \r\n\r\nOK\r\n at tail */
	handleSize = (size_t)(strlen((char*)temp) - 9);
	/* Data start from temp[1], temp[0] is white space */
	memcpy(handleID, &temp[1], handleSize);

	memset(cmd, 0, 128);
	dataSize = (int)(strlen((char*)data));
	sprintf((char*)cmd, "AT+QFWRITE=%s,%d", handleID, dataSize);
	if(AT_sendCheckReply(cmd, (uint8_t*)"CONNECT", AT_TIMEOUT_5000MS))
	{   /* Open file success */
		if(AT_expectReplyOK(data, AT_TIMEOUT_5000MS))
		{	/* Write file success */
			/* Do nothing */
		}
	}

	/* Write success/Write fail */
	memset(cmd, 0, 128);
	sprintf((char*)cmd, "AT+QFCLOSE=%s", handleID);
	return (AT_expectReplyOK(cmd, AT_TIMEOUT_5000MS));
}


/* Extend from previous file data */
bool UC200_FileWrite(uint8_t *FileName, uint8_t *data)
{
	uint8_t cmd[128] = {0};
	uint8_t temp[64] = {0};
	uint8_t handleID[32] = {0};
	size_t  handleSize = 0;
	int     dataSize = 0;

	sprintf((char*)cmd, "AT+QFOPEN=\"%s\",0", (char*)FileName);
	if (!AT_expectReplyOK(cmd, AT_TIMEOUT_5000MS))
	{
		return false;
	}
	AT_returnString(temp, 1, (uint8_t*)":");
	/* magic number 9: white space at head and response \r\n\r\nOK\r\n at tail */
	handleSize = (size_t)(strlen((char*)temp) - 9);
	/* Data start from temp[1], temp[0] is white space */
	memcpy(handleID, &temp[1], handleSize);

	/* Seek file to end point */
	memset(cmd, 0, 128);
	dataSize = (int)(strlen((char*)data));
	sprintf((char*)cmd, "AT+QFSEEK=%s,0,2", handleID);
	if(AT_expectReplyOK(cmd, AT_TIMEOUT_1000MS))
	{
		memset(cmd, 0, 128);
		dataSize = (int)(strlen((char*)data));
		sprintf((char*)cmd, "AT+QFWRITE=%s,%d", handleID, dataSize);
		if(AT_sendCheckReply(cmd, (uint8_t*)"CONNECT", AT_TIMEOUT_2000MS))
		{   /* Open file success */
			if(AT_expectReplyOK(data, AT_TIMEOUT_2000MS))
			{	/* Write file success */
				/* Do nothing */
			}
		}
	}
	/* Write success/Write fail */
	memset(cmd, 0, 128);
	sprintf((char*)cmd, "AT+QFCLOSE=%s", handleID);
	return (AT_expectReplyOK(cmd, AT_TIMEOUT_1000MS));
}

bool UC200_FileRead(uint8_t *FileName, uint8_t *OutBuffer, uint32_t len)
{
	uint8_t cmd[128] = {0};
	uint8_t temp[64] = {0};
	uint8_t handleID[32] = {0};
	size_t  handleSize = 0;

	if(len > AT_CORE_BUFFER_SIZE)
	{
		return false;
	}

	sprintf((char*)cmd, "AT+QFOPEN=\"%s\",2", (char*)FileName);
	if (!AT_expectReplyOK(cmd, AT_TIMEOUT_5000MS))
	{
		return false;
	}
	AT_returnString(temp, 1, (uint8_t*)":");
	/* magic number 9: white space at head and response \r\n\r\nOK\r\n at tail */
	handleSize = (size_t)(strlen((char*)temp) - 9);
	/* Data start from temp[1], temp[0] is white space */
	memcpy(handleID, &temp[1], handleSize);

	/* Seek file to beginning point */
	memset(cmd, 0, 128);
	sprintf((char*)cmd, "AT+QFSEEK=%s,0,0", handleID);
	if(AT_expectReplyOK(cmd, AT_TIMEOUT_5000MS))
	{
		memset(cmd, 0, 128);
		sprintf((char*)cmd, (char*)"AT+QFREAD=%s,%d", handleID, (int)len);
		if(AT_expectReplyOK(cmd, AT_TIMEOUT_5000MS))
		{
			AT_returnString(OutBuffer, 1, (uint8_t*)"\r\n");
		}
	}

	/* Write success/Write fail */
	memset(cmd, 0, 128);
	sprintf((char*)cmd, "AT+QFCLOSE=%s", handleID);
	return (AT_expectReplyOK(cmd, AT_TIMEOUT_1000MS));
}

bool UC200_FileDelete(uint8_t *FileName)
{
	uint8_t cmd[128] = {0};
	sprintf((char*)cmd, "AT+QFDEL=\"%s\"", (char*)FileName);

	/* If no available file, assume delete success */
	if(!UC200_FileCheck(FileName))
	{
		return true;
	}

	return (AT_expectReplyOK(cmd, AT_TIMEOUT_10000MS));
}

/* This function slow when deal with big file and read by chunk entire of file */
bool UC200_FileReadChunk(uint8_t *FileName, uint8_t *OutBuffer, uint32_t skip, uint32_t len)
{
	uint8_t cmd[128] = {0};
	uint8_t temp[64] = {0};
	uint8_t handleID[32] = {0};
	size_t  handleSize = 0;
	uint8_t *ptr = NULL;
	uint16_t loopCount = 0;
	uint32_t copyLen = 0;

	if(len > AT_CORE_BUFFER_SIZE)
	{
		return false;
	}

	sprintf((char*)cmd, "AT+QFOPEN=\"%s\",2", (char*)FileName);
	if (!AT_expectReplyOK(cmd, AT_TIMEOUT_5000MS))
	{
		return false;
	}
	AT_returnString(temp, 1, (uint8_t*)":");
	/* magic number 9: white space at head and response \r\n\r\nOK\r\n at tail */
	handleSize = (size_t)(strlen((char*)temp) - 9);
	/* Data start from temp[1], temp[0] is white space */
	memcpy(handleID, &temp[1], handleSize);

	/* Seek file to beginning point */
	memset(cmd, 0, 128);
	sprintf((char*)cmd, "AT+QFSEEK=%s,%d,0", handleID, (int)skip);
	if(AT_expectReplyOK(cmd, AT_TIMEOUT_5000MS))
	{
		memset(cmd, 0, 128);
		sprintf((char*)cmd, (char*)"AT+QFREAD=%s,%d", handleID, (int)len);
		if(AT_expectReplyOK(cmd, AT_TIMEOUT_5000MS))
		{
			/* ***************************************
			 * The response will on this format
			 * \r\nCONNECT 10\r\nREAL--DATA\r\nOK\r\n
			 * So, need to start copy after second \n
			 * ****************************************/
			ptr = (uint8_t*)&_STORAGE.RxBuffer[2];
			loopCount = 2;
			while(*ptr != '\n')
			{
				ptr++;
				loopCount++;

				/* Exit if over buffer */
				if(loopCount >= _STORAGE.RxIndex)
				{
					break;
				}
			}

			/* Target found */
			if(loopCount < _STORAGE.RxIndex)
			{
				while(copyLen < len)
				{
					OutBuffer[copyLen] = *(++ptr);
					copyLen++;
				}
			}

		}
	}

	/* Write success/Write fail */
	memset(cmd, 0, 128);
	sprintf((char*)cmd, "AT+QFCLOSE=%s", handleID);
	return (AT_expectReplyOK(cmd, AT_TIMEOUT_1000MS));
}


uint32_t UC200_FileOpen(uint8_t *FileName)
{
	uint8_t cmd[128] = {0};
	uint8_t temp[64] = {0};
	uint8_t handleID[32] = {0};
	size_t  handleSize = 0;

	sprintf((char*)cmd, "AT+QFOPEN=\"%s\",0", (char*)FileName);
	if (!AT_expectReplyOK(cmd, AT_TIMEOUT_5000MS))
	{
		return false;
	}
	AT_returnString(temp, 1, (uint8_t*)":");
	/* magic number 9: white space at head and response \r\n\r\nOK\r\n at tail */
	handleSize = (size_t)(strlen((char*)temp) - 9);
	/* Data start from temp[1], temp[0] is white space */
	memcpy(handleID, &temp[1], handleSize);

	return ((uint32_t)(strtol((char*)handleID, NULL, 10)));
}

bool UC200_FileReadContinuous(uint32_t handleID, uint8_t *OutBuffer, uint32_t readsize)
{
	uint8_t cmd[128] = {0};
	uint8_t *ptr = NULL;
	uint16_t loopCount = 0;
	uint32_t copyLen = 0;

	sprintf((char*)cmd, (char*)"AT+QFREAD=%ld,%d", handleID, (int)readsize);
	if(!AT_expectReplyOK(cmd, AT_TIMEOUT_5000MS))
	{
		return false;
	}
	else
	{
		/* ***************************************
		 * The response will on this format
		 * \r\nCONNECT 10\r\nREAL--DATA\r\nOK\r\n
		 * So, need to start copy after second \n
		 * ****************************************/
		ptr = (uint8_t*)&_STORAGE.RxBuffer[2];
		loopCount = 2;
		while(*ptr != '\n')
		{
			ptr++;
			loopCount++;

			/* Exit if over buffer */
			if(loopCount >= _STORAGE.RxIndex)
			{
				break;
			}
		}

		/* Target found */
		if(loopCount < _STORAGE.RxIndex)
		{
			while(copyLen < readsize)
			{
				OutBuffer[copyLen] = *(++ptr);
				copyLen++;
			}
		}

	}

	return true;
}

bool UC200_FileClose(uint32_t handleID)
{
	uint8_t cmd[128] = {0};

	sprintf((char*)cmd, "AT+QFCLOSE=%ld", handleID);
	return (AT_expectReplyOK(cmd, AT_TIMEOUT_1000MS));
}

bool UC200_FileCheck(uint8_t *FileName)
{
	uint8_t cmd[128] = {0};
	sprintf((char*)cmd, "AT+QFLST=%s", FileName);
	return(AT_expectReplyOK(cmd, AT_TIMEOUT_1000MS));
}

uint32_t UC200_FileGetSize(uint8_t *FileName)
{
	uint8_t cmd[128] = {0};
	uint8_t sizeDigit[32] = {0};
	size_t  sizeDigitLen = 0;
	uint32_t fileSize = 0;
	sprintf((char*)cmd, "AT+QFLST=%s", FileName);

	if(!AT_expectReplyOK(cmd, AT_TIMEOUT_1000MS))
	{
		fileSize = 0;
	}
	else
	{
		/* Get string size and cleanup \r\n\r\nOK\r\n */
		AT_returnString(sizeDigit, 1, (uint8_t*)",");
		sizeDigitLen = strlen((char*)sizeDigit) - (size_t)(8);
		sizeDigit[sizeDigitLen] = 0;

		fileSize = (uint32_t)strtol((char*)sizeDigit, NULL, 10);
	}

	return fileSize;
}

bool UC200_EnterSleepMode(void){
	bool status = AT_expectReplyOK("AT+QSCLK=1", AT_TIMEOUT_5000MS);
	HAL_GPIO_WritePin(STM_DTR_GSM_GPIO_Port,STM_DTR_GSM_Pin,GPIO_PIN_SET);
	return status;
}

bool UC200_ExitSleepMode(void){
	HAL_GPIO_WritePin(STM_DTR_GSM_GPIO_Port,STM_DTR_GSM_Pin,GPIO_PIN_SET);
	return UC200_syncModule(2000U);
}
