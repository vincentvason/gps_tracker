/*
 * proc_data.h
 *
 *  Created on: Aug 25, 2020
 *      Author: Vason-PC
 */

#ifndef PROC_DATA_H_
#define PROC_DATA_H_

#include <stdio.h>
#include <string.h>

#include "usart.h"
#include "denso_ram.h"

typedef struct{
	uint8_t pack_no[8];
	uint8_t  imei[24];
	uint8_t timestamp[16];
	uint8_t  RAMString[264];
	uint8_t	 lat[16];
	uint8_t	 lng[16];
	uint8_t adc[8];
}PACKET_t;

typedef union {
    struct {
        unsigned FlagGPSDetached: 1;
        unsigned FlagConnectionError: 1;
        unsigned FlagRAM2Error: 1;
        unsigned FlagRAM3Error: 1;
        unsigned FlagRAM4Error: 1;
        unsigned FlagReserved2: 1;
        unsigned FlagReserved1: 1;
        unsigned FlagReserved0: 1;
    };
    uint8_t AllFlags;
}Data_Flag_t;

typedef struct{
	uint8_t  id_imei[24];
	uint8_t  imei[16];
	uint8_t  ccid[24];
	uint16_t pack_no;
	uint64_t timestamp;
	double	 lat;
	double	 lng;
	uint32_t adc;
	Data_Flag_t flag;
	uint8_t ram2[129];
	uint8_t ram3[129];
	uint8_t ram4[129];
	uint8_t ram2_length;
	uint8_t ram3_length;
	uint8_t ram4_length;
}DATA_t;

typedef union {
    struct {
        unsigned ReservedBit: 1;
    	unsigned FlagRAM2_ChecksumError: 1;
        unsigned FlagRAM2_FrameError: 1;
        unsigned FlagRAM2_NoiseError: 1;
        unsigned FlagRAM2_RetryTime: 2;
        unsigned FlagRAM3_ChecksumError: 1;
        unsigned FlagRAM3_FrameError: 1;
        unsigned FlagRAM3_NoiseError: 1;
        unsigned FlagRAM3_RetryTime: 2;
        unsigned FlagRAM4_ChecksumError: 1;
        unsigned FlagRAM4_FrameError: 1;
        unsigned FlagRAM4_NoiseError: 1;
        unsigned FlagRAM4_RetryTime: 2;
    };
    uint16_t AllFlags;
}ST_RAM_t;

typedef struct{
	uint64_t  timestamp_begin;
	uint64_t  timestamp_end;
	uint8_t	  fail_time;
	uint8_t   gps_fail;
	uint8_t   operation_time;
	ST_RAM_t  Flags;
}STATUS_t;


extern DATA_t _DATA;
extern STATUS_t _STATUS;
extern uint8_t _DATAString[1500]; //must same as core at buffer

void GenerateID(void);
void PackageData(void);

#endif /* PROC_DATA_H_ */
