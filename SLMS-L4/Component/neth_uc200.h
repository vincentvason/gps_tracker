/*
 * neth_UC200.h
 *
 *  Created on: Apr 23, 2020
 *      Author: Saifa-NB
 */

#ifndef INC_NETH_UC200_H_
#define INC_NETH_UC200_H_

#include "main.h"
#include "usart.h"
#include "neth_ATparser.h"

#include <string.h>
#include <inttypes.h>

/* Mobile Country Codes (MCC) and Mobile Network Codes (MNC) */
/* https://en.wikipedia.org/wiki/Mobile_country_code */
typedef enum
{
	/* Thailand */
	MMCMNC_TrueByCAT  = (uint16_t)(52000),
	MMCMNC_AIS	      = (uint16_t)(52003),
	MMCMNC_TrueMove	  = (uint16_t)(52004),
	MMCMNC_DTAC 	  = (uint16_t)(52005),
	MMCMNC_TOT	      = (uint16_t)(52015)
}MccMnc_enumList;

typedef enum
{
	/****************/
	/* Working Mode */
	/****************/
	DEACTIVE = (uint8_t)(0),
	ACTIVE   = (uint8_t)(1),

	/*******************/
	/* Operator status */
	/*******************/
	OPR_NOT_WORKING      = (uint8_t)(0),
	OPR_REGISTERD_HOME   = (uint8_t)(1),
	OPR_SEARCHING        = (uint8_t)(2),
	OPR_REGISTER_DENIED  = (uint8_t)(3),
	OPR_UNKNOWN          = (uint8_t)(4),
	OPR_ROAMING          = (uint8_t)(5),

	/******************/
	/* SIM functional */
	/******************/
	SIM_MODE_MINIMUM_FUNCTION = (uint8_t)(0),   /* Minimum functionality */
	SIM_MODE_FULL_FUNCTION    = (uint8_t)(1),   /* Full functionality **Default** */
	SIM_MODE_DISABLE_RX_RF    = (uint8_t)(3),   /* Disable RF, from receiving signal */
	SIM_MODE_DISABLE_TX_RX_RF = (uint8_t)(4),   /* Disable RF, both transmitting and receiving signal */

	SIM_RST_DISABLE_SIM       = (uint8_t)(5),   /* Disable SIM card */
	SIM_RST_NONE              = (uint8_t)(0),   /* Not reset module **Default** */
	SIM_RST_MODULE_RST        = (uint8_t)(1),   /* Reset module and set functional (CFUN) to 1 after reset */

	/******************/
	/* Preferred mode */
	/******************/
	PREFERRED_2G_3G = (uint8_t)(0),
	PREFERRED_2G    = (uint8_t)(1),
	PREFERRED_3G    = (uint8_t)(2),

	PREFERRED_RST_APPLY = (uint8_t)(0),
	PREFERRED_NOW_APPLY = (uint8_t)(1),

	/*************/
	/* Buad rate */
	/*************/
	BAUD_4800    = (uint32_t)(4800),
	BAUD_9600    = (uint32_t)(9600),
	BAUD_19200   = (uint32_t)(19200),
	BAUD_38400   = (uint32_t)(38400),
	BAUD_57600   = (uint32_t)(57600),
	BAUD_115200  = (uint32_t)(115200), /* Default */
	BAUD_230400  = (uint32_t)(230400),
	BAUD_460800  = (uint32_t)(460800),
	BAUD_921600  = (uint32_t)(921600),
	BAUD_1000000 = (uint32_t)(1000000),

	/*******************/
	/* Operator Format */
	/*******************/
	OPR_FORMAT_LONG_NAME  = (uint8_t)(0), /* Default */
	OPR_FORMAT_SHORT_NAME = (uint8_t)(1),
	OPR_FORMAT_MCCMNC     = (uint8_t)(2)
}UC200T_enumList;

typedef struct
{
	uint8_t apnName[32];
	uint8_t apnUser[32];
	uint8_t apnPass[32];

	uint8_t serverIP[32];
	uint16_t serverPort;
	uint8_t serverName[32];
	uint8_t serverPath[32];
	uint8_t serverUser[32];
	uint8_t serverPass[32];
}UC200_Conf;


	void (*UC200_RxCallback)(void);

//	bool AGPSinit1(void);
//	bool AGPSinit2(void);

	/* Initialize */
	bool UC200_init(UART_HandleTypeDef *huart);

	/* Checking  */
	bool UC200_syncModule(uint32_t timeOut);

	/* miscellaneous settings */
	bool UC200_echoCommand(uint8_t  mode);
	bool UC200_echoErrorMessage(uint8_t mode);
	bool UC200_setBaudrate(uint32_t baud);


	/* request general information */
	uint8_t UC200_getCCID(uint8_t *buffer);
	uint8_t UC200_getIMEI(uint8_t *buffer);
	uint8_t UC200_getCSQ(void);


	/* SIM card functional */
	bool UC200_setFunctionality(uint8_t option, uint8_t reset);


	/* Network */
	bool UC200_setFormatOperator(uint8_t format);
	bool UC200_deRegistered(uint8_t mode);
	bool UC200_setPreferredScan(uint8_t mode, uint8_t apply);
	bool UC200_setPreferredPriority(uint8_t mode, uint8_t apply);
	uint8_t UC200_getNetworkStatus(uint8_t mode);


	/* Internet */
	bool UC200_GPRSsetParam(uint8_t* APN, uint8_t* User, uint8_t* Pass);
	bool UC200_GPRSgetParamFromList(void);
	bool UC200_GPRSenable(uint8_t mode);
	bool UC200_GPRScheckIP(void);


	/* Time */
	bool UC200_RTCenableTimeSync(uint8_t mode);
	bool UC200_RTCread(uint8_t* buffer);
	bool UC200_ConvertEpochTime(char* str, uint64_t* epoch_time);



	/* TCP/IP */
	bool UC200_TcpipOpen(uint8_t* server, int port);
	bool UC200_TcpipClose(void);
	bool UC200_TcpipSend(uint8_t* msg, int len);
	bool UC200_TcpipReceive(uint16_t len);

	/* MQTT */
	void UC200_MqttConnectMessage (uint8_t*msg, const uint8_t* id, const uint8_t* user, const uint8_t* pass);
	void UC200_MqttPublishMessage(uint8_t* msg, const uint8_t* topic, const uint8_t* data);
	bool UC200_MqttSendPacket(uint8_t *packet, int len);


	/* MQTT application layer Will move later */
	bool UC200_MQTTConnect(const uint8_t *protocol, const uint8_t *clientID, const uint8_t *username, const uint8_t *password);
	bool UC200_MQTTConnectCheck(uint8_t *clientID, uint8_t *username, uint8_t *password);
	bool UC200_MQTTPing(void);
	bool UC200_MQTTPingCheck(void);
	bool UC200_MQTTPublish(const uint8_t* topic, const uint8_t* message);


	/* File system */
	bool UC200_Filelist(void);
	bool UC200_FileNew(uint8_t *FileName, uint8_t *data);
	bool UC200_FileWrite(uint8_t *FileName, uint8_t *data);
	bool UC200_FileRead(uint8_t *FileName, uint8_t *OutBuffer, uint32_t len);
	bool UC200_FileDelete(uint8_t *FileName);
	bool UC200_FileReadChunk(uint8_t *FileName, uint8_t *OutBuffer, uint32_t skip, uint32_t len);
	bool UC200_FileCheck(uint8_t *FileName);
	uint32_t UC200_FileGetSize(uint8_t *FileName);
	bool UC200_FileDeleteAll(void); /* Special case */

	/* Faster than UC200_FileReadChunk when read the same file many time */
	uint32_t UC200_FileOpen(uint8_t *FileName);
	bool UC200_FileReadContinuous(uint32_t handleID, uint8_t *OutBuffer, uint32_t readsize);
	bool UC200_FileClose(uint32_t handleID);


	/* HTTP */
//	bool HTTPsetup(uint8_t *ip,uint8_t *name, uint16_t port, uint8_t *usr, uint8_t *pwd);
//	bool HTTPfilePOST(uint8_t* Filename, uint8_t *path);


//	bool reternURDFILE(uint8_t *result, int size);
//	bool reternURDBLOCK(uint8_t *result);

	/* Power Mode */
	bool UC200_EnterSleepMode(void);
	bool UC200_ExitSleepMode(void);



#endif /* INC_NETH_UC20_H_ */
