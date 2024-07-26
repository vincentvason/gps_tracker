/*
 * neth_gnss.h
 *
 *  Created on : Apr 15, 2020
 *      Author : Saifa K.
 *      Version: 1.00
 *      Edited : 2020-Apr-15
 *  Note.
 *  Code based on xmc1000_gps from Kittipong T.
 */
#ifndef NETH_GNSS_H
#define NETH_GNSS_H


#include "main.h"
#include "usart.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>

#define GNSS_BUFFER_LENGTH	90

#define GNSS_ECHO_MODE			0
#if(GNSS_ECHO_MODE)
#define GNSS_DBG_UART			huart1
#endif

#define GNSS_DBG_BUFFER         0



typedef enum{
	/* NMEA Format support */
	$GPRMC = 0,
	$GPVTG,
	$GPGGA,
	$GPGSA,
	$GPGSV,
	$GPGLL,
	$GPTXT,

	/* Checksum */
	CHECKSUM_NG = 0,
	CHECKSUM_OK,

	/* Speed unit */
	SPEED_KNOT = 0,
	SPEED_HMPH
}GPS_enumList;


typedef struct{
	/***************************/
	/* General GPS information */
	/***************************/
	/* GNRMC */
	double latitude;
	double longitude;
	char fixedStatus;
	int date;
	int utcTime;

	/* GNGGA: Fixed information */
	int fixQuality;
	int numSatellites;
	float altitude;
	float height;

	/* GNGSA: Overall satellite data */
	float PDOP;
	float HDOP;
	float VDOP;

	/* GNVTG */
	float speed_N;
	float speed_k;

	/* DataReady */
	uint8_t IdleCount;

}GNSS_Data;


extern bool GNSS_DataReady;

/**************************/
/* I n i t i a l i z e    */
/**************************/
	bool  gnss_init(UART_HandleTypeDef *huart);
	void  gnss_reset(void);

/**************************/
/* g e t                  */
/**************************/
	uint8_t  gnss_getNMEAformat(void);

	/* GPS data */
	double gnss_getLatitude(void);
	double gnss_getLongitude(void);
	char   gnss_getFixedStatus(void);
	int    gnss_getDate(void);
	int    gnss_getUTCtime(void);
	int    gnss_getFixedQuality(void);
	int    gnss_getNumofSatellite(void);
	float  gnss_getAltitude(void);
	float  gnss_getHeight(void);
	float  gnss_getPDOP(void);
	float  gnss_getHDOP(void);
	float  gnss_getVDOP(void);
	float  gnss_getSpeedOverGround(uint8_t unit);


/**************************/
/* s e t                  */
/**************************/
	void gnss_setNMEAformat(uint8_t NmeaFormat);
	void gnss_updatePosition(void);

/**************************/
/* C a l l b a c k        */
/**************************/
	void gnss_RxCallback(void);
	void gnss_RxITEnable(void);
	void gnss_RxITDisable(void);

	void gnss_chartoBuffer(char rx_data);
	bool gnss_ChecksumVerify(uint8_t *Data);
	void gnss_ExtractData(void);

#endif
