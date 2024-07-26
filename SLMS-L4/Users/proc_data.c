/*
 * proc_data.c
 *
 *  Created on: Aug 25, 2020
 *      Author: Vason-PC
 */

#include "proc_data.h"

#define dbg_print(...)     HAL_UART_Transmit(&huart1, (uint8_t*)__VA_ARGS__, strlen(__VA_ARGS__), 1000)

const char vers[8] = "1.4.18";
STATUS_t _STATUS;
DATA_t _DATA;
uint8_t _DATAString[1500]; //must same as at core buffer
uint8_t temp[32];

void IncludeDiagnostic(){
	int this_temp = 0;
	if(_DATA.flag.FlagGPSDetached == true){
		strcat((char*)_DATAString,"_GE");
	}
	if(_STATUS.fail_time > 0){
		strcat((char*)_DATAString,"_CNE");
	}
//	sprintf(temp,"_RRT%d%d%d",_STATUS.Flags.FlagRAM2_RetryTime,_STATUS.Flags.FlagRAM3_RetryTime,_STATUS.Flags.FlagRAM4_RetryTime);

	this_temp = (_STATUS.Flags.FlagRAM2_FrameError << 2);
	this_temp += (_STATUS.Flags.FlagRAM3_FrameError << 1);
	this_temp += (_STATUS.Flags.FlagRAM4_FrameError << 0);
	if(this_temp > 0){
		sprintf(temp,"_RFE%d",this_temp);
		strcat((char*)_DATAString,temp);
	}

	this_temp = (_STATUS.Flags.FlagRAM2_NoiseError << 2);
	this_temp += (_STATUS.Flags.FlagRAM3_NoiseError << 1);
	this_temp += (_STATUS.Flags.FlagRAM4_NoiseError << 0);
	if(this_temp > 0){
		sprintf(temp,"_RNE%d",this_temp);
		strcat((char*)_DATAString,temp);
	}

	this_temp = (_STATUS.Flags.FlagRAM2_ChecksumError << 2);
	this_temp += (_STATUS.Flags.FlagRAM3_ChecksumError << 1);
	this_temp += (_STATUS.Flags.FlagRAM4_ChecksumError << 0);
	if(this_temp > 0){
		sprintf(temp,"_RCE%d",this_temp);
		strcat((char*)_DATAString,temp);
	}

}

void GenerateID(void){
	sprintf((char*)_DATA.id_imei,"I%s",(char*)_DATA.imei);
}

void PackageData(void){
	PACKET_t _PACKET;
	//SECTION: Format Change
	sprintf((char*)_PACKET.pack_no,"%d",_DATA.pack_no);
	sprintf((char*)_PACKET.imei,"\"%s\"",(char*)_DATA.imei);
	sprintf((char*)_PACKET.timestamp,"%ld",_DATA.timestamp);

	sprintf((char*)_PACKET.lat,"%ld",(uint32_t)(_DATA.lat*1000000));
	sprintf((char*)_PACKET.lng,"%ld",(uint32_t)(_DATA.lng*1000000));

	float adc_float = ((_DATA.adc)/(4095.0/1.8));
	adc_float = (adc_float/(1.6/4.2));
	adc_float = adc_float - 0.2;
	sprintf((char*)_PACKET.adc,"%d",(uint16_t)(adc_float*1000));

	uint8_t RAMByte[3];

	//SECTION: String Concentrate
	strcpy((char*)_DATAString,"(");
	strcat((char*)_DATAString,(char*)_PACKET.imei);
	strcat((char*)_DATAString,",");
	strcat((char*)_DATAString,(char*)_PACKET.pack_no);
	strcat((char*)_DATAString,",");
	strcat((char*)_DATAString,(char*)_PACKET.timestamp);
	strcat((char*)_DATAString,",");
	strcat((char*)_DATAString,(char*)_PACKET.lat);
	strcat((char*)_DATAString,",");
	strcat((char*)_DATAString,(char*)_PACKET.lng);
	strcat((char*)_DATAString,",");
	strcat((char*)_DATAString,(char*)_PACKET.adc);
	strcat((char*)_DATAString,",");

	/** Version and Debugging */
	strcat((char*)_DATAString,(char*)vers);
	IncludeDiagnostic();


	strcat((char*)_DATAString,",");

	/**Ram Page 2**/
	memset(_PACKET.RAMString,0,264);
	if(_DATA.ram2_length < 129){
		memset(_PACKET.RAMString,'0',129*2);
	}
	else{
		for(int i = 0; i < 129; i++){
			sprintf((char*)RAMByte,"%02x",_DATA.ram2[i]);
			strcat((char*)_PACKET.RAMString,(char*)RAMByte);
		}
	}
	strcat((char*)_DATAString,(char*)_PACKET.RAMString);
	strcat((char*)_DATAString,",");

	/**Ram Page 3**/
	memset(_PACKET.RAMString,0,264);
	if(_DATA.ram3_length < 129){
		memset(_PACKET.RAMString,'0',129*2);
	}
	else{
		for(int i = 0; i < 129; i++){
			sprintf((char*)RAMByte,"%02x",_DATA.ram3[i]);
			strcat((char*)_PACKET.RAMString,(char*)RAMByte);
		}
	}
	strcat((char*)_DATAString,(char*)_PACKET.RAMString);
	strcat((char*)_DATAString,",");

	/**Ram Page 4**/
	memset(_PACKET.RAMString,0,264);
	if(_DATA.ram4_length < 129){
		memset(_PACKET.RAMString,'0',129*2);
	}
	else{
		for(int i = 0; i < 129; i++){
			sprintf((char*)RAMByte,"%02x",_DATA.ram4[i]);
			strcat((char*)_PACKET.RAMString,(char*)RAMByte);
		}
	}
	strcat((char*)_DATAString,(char*)_PACKET.RAMString);

	strcat((char*)_DATAString,")");
	dbg_print((char*)_DATAString);
}

