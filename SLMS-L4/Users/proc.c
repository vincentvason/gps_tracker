/*
 * proc.c
 *
 *  Created on: Aug 25, 2020
 *      Author: Vason-PC
 */
#include "proc.h"
#include "denso_ram.h"

#define START_FRESH		 0
#define RAM_RETRY		 2
#define	RAM_TIMEOUT		 750

#define GPS_ENABLE       1
#define UC200_ENABLE     1
#define OLED_ENABLE      1
#define DENSORAM_ENABLE  1
#define BATTADC_ENABLE	 1


float adc_float;

uint8_t sub_room[32] = {0};
uint8_t u8check;
uint16_t u16check;
uint8_t txtBuffer[256];
uint8_t debugBuffer[256];
int16_t wait_loop;
uint8_t state;

void I2C_Scanner(void);

void Setup(void) {
	/** SECTION: OLED */
	state = 1;

#if(OLED_ENABLE)
	HAL_GPIO_WritePin(SENSOR_PWR_CTRL_GPIO_Port, SENSOR_PWR_CTRL_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(OPTIGA_RST_GPIO_Port, OPTIGA_RST_Pin, GPIO_PIN_RESET);
	HAL_Delay(10);
	HAL_GPIO_WritePin(SENSOR_PWR_CTRL_GPIO_Port, SENSOR_PWR_CTRL_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(OPTIGA_RST_GPIO_Port, OPTIGA_RST_Pin, GPIO_PIN_SET);
	HAL_Delay(1000);
	ssd1306_Init(&hi2c2);
	HAL_Delay(10);
	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString("Hello          ", Font_7x10, White);
	ssd1306_UpdateScreen();
	HAL_Delay(1000);

#endif
	HAL_IWDG_Refresh(&hiwdg);

	/** SECTION: GPS */
	state = 2;
#if(GPS_ENABLE)
#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString("GPS Init      ", Font_7x10, White);
	ssd1306_UpdateScreen();
#endif
	gnss_init(&huart3);
	gnss_RxITDisable();
#endif
	HAL_IWDG_Refresh(&hiwdg);

	/** SECTION: DENSO RAM */
	state = 3;
#if(DENSORAM_ENABLE)
#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString("RAM Init      ", Font_7x10, White);
	ssd1306_UpdateScreen();
#endif
	DENSORAM_Init(&huart1);
	DENSORAM_RxITDisable();
#endif
	HAL_IWDG_Refresh(&hiwdg);

	/** SECTION: UC200 */
	state = 4;
#if(UC200_ENABLE)
#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString("UC200T Init    ", Font_7x10, White);
	ssd1306_UpdateScreen();
#endif
	UC200_init(&huart4);

	/* Get function test */
	state = 5;
#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 10);
	ssd1306_WriteString("Get IMEI and CCID   ", Font_7x10, White);
	ssd1306_UpdateScreen();
#endif
	u8check = UC200_getIMEI(_DATA.imei);
	GenerateID();
	u8check &= UC200_getCCID(_DATA.ccid);

	/* SIM available */
	state = 6;
	if (u8check != 0) {
#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 10);
	ssd1306_WriteString("Connecting...      ", Font_7x10, White);
	ssd1306_UpdateScreen();
#endif
		UC200_setFunctionality(SIM_MODE_FULL_FUNCTION, SIM_RST_NONE);
		UC200_setPreferredScan(PREFERRED_3G, PREFERRED_NOW_APPLY);
		UC200_setPreferredPriority(PREFERRED_3G, PREFERRED_NOW_APPLY);
		HAL_IWDG_Refresh(&hiwdg);
		/* Wait until connected operator */
		state = 7;
#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 10);
	ssd1306_WriteString("Get Network Status    ", Font_7x10, White);
	ssd1306_UpdateScreen();
#endif
		for (int i = 0; i < 150; i++) {
			HAL_Delay(1000);
			HAL_IWDG_Refresh(&hiwdg);
			if(UC200_getNetworkStatus(PREFERRED_2G_3G) == OPR_REGISTERD_HOME) break;
		}
		HAL_IWDG_Refresh(&hiwdg);

		/* Internet function test */
		state = 8;
#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 10);
	ssd1306_WriteString("Enable PDP          ", Font_7x10, White);
	ssd1306_UpdateScreen();
#endif
		UC200_GPRSgetParamFromList();
		UC200_GPRSenable(ACTIVE);
		UC200_GPRScheckIP();
#if(START_FRESH)
#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 10);
	ssd1306_WriteString("Start Fresh         ", Font_7x10, White);
	ssd1306_UpdateScreen();
#endif
		UC200_DeleteFile("*");
		WRITE_SAVEINDEX(0);
#else
#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 10);
	ssd1306_WriteString("Retrieve Save File      ", Font_7x10, White);
	ssd1306_UpdateScreen();
	sprintf((char*)txtBuffer, "Save index is %d         ",save_index);
	ssd1306_SetCursor(0, 20);
	ssd1306_WriteString((char*)txtBuffer, Font_7x10, White);
	ssd1306_UpdateScreen();
#endif
		dbg_println("\033[1;36mRetrieve Save index from backup register\033[0m");
		save_index = READ_SAVEINDEX();
		sprintf((char*) debugBuffer, "\033[0;36mSave index now is %d\033[0m", save_index);
		dbg_println(debugBuffer);
#endif
		state = 81;
#if(OLED_ENABLE)
		ssd1306_SetCursor(0, 10);
		ssd1306_WriteString("MQTT Connect      ", Font_7x10, White);
		ssd1306_UpdateScreen();
#endif
		u8check = UC200_TcpipOpen((uint8_t*) "3.1.176.170", 1883);
		u8check = UC200_MQTTConnectCheck(_DATA.id_imei,(uint8_t*)"acov",(uint8_t*)"devrnd");
		sprintf(sub_room,"ctl/%s/",_DATA.imei);
//		u8check &= UC200T_MQTTsubscribe(sub_room, 0);
	} else {
#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 10);
	ssd1306_WriteString("Connect Fail            ", Font_7x10, White);
	ssd1306_SetCursor(0, 20);
	ssd1306_WriteString("Try to Reset            ", Font_7x10, White);
	ssd1306_UpdateScreen();
	HAL_Delay(1000);
#endif
		NVIC_SystemReset();
	}
#endif
	HAL_IWDG_Refresh(&hiwdg);

	_DATA.pack_no = 0;
}

void Loop(void) {
	/** SECTION: Get Time from UC200 */
	state = 9;
#if(UC200_ENABLE)
#if(OLED_ENABLE)
	ssd1306_Init(&hi2c2);
	ssd1306_UpdateScreen();
	sprintf((char*)txtBuffer, "Packet No.%d           ",_DATA.pack_no);
	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString((char*)txtBuffer, Font_7x10, White);
	ssd1306_SetCursor(0, 10);
	ssd1306_WriteString("Begin                 ", Font_7x10, White);
	ssd1306_UpdateScreen();
#endif
	dbg_println("\033[1;36mBegin Get Time\033[0m");
	u8check = UC200_RTCread(txtBuffer);
	u8check &= UC200_ConvertEpochTime((char*) txtBuffer, &(_DATA.timestamp));
	_STATUS.timestamp_begin = _DATA.timestamp;
#endif
	HAL_IWDG_Refresh(&hiwdg);

	if(_STATUS.fail_time > 1){
		u8check = UC200_TcpipClose();
		u8check &= UC200_TcpipOpen((uint8_t*) "3.1.176.170", 1883);
		u8check &= UC200_MQTTConnectCheck(_DATA.id_imei,(uint8_t*)"acov",(uint8_t*)"devrnd");
//		u8check &= UC200T_MQTTsubscribe(sub_room, 0);
	}

	/** SECTION: ADC */
	state = 10;
#if(BATTADC_ENABLE)
#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 10);
	ssd1306_WriteString("ADC Measure...         ", Font_7x10, White);
	ssd1306_UpdateScreen();
#endif
	dbg_println("\033[1;36mBegin Get ADC Value\033[0m");
	ADC_ChannelConfig(ADC_CHANNEL_8);
	HAL_GPIO_WritePin(BATT_ADC_EN_GPIO_Port, BATT_ADC_EN_Pin, 1);
	HAL_Delay(500);
	HAL_ADC_Start(&hadc1);
	for (int i = 0; i < 100; i++) {
		if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
			break;
	}
	_DATA.adc = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);
	HAL_GPIO_WritePin(BATT_ADC_EN_GPIO_Port, BATT_ADC_EN_Pin, 0);
	HAL_IWDG_Refresh(&hiwdg);

#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 10);
	adc_float = ((_DATA.adc)/(4095.0/1.8));
	adc_float = (adc_float/(1.6/4.2));
	adc_float = adc_float - 0.2;
	if(!HAL_GPIO_ReadPin(CHARGE_STATUS_GPIO_Port, CHARGE_STATUS_Pin)){
		sprintf((char*)txtBuffer, "ADC %.2fV Charged",adc_float);
		dbg_println("\033[0;36mCharged State\033[0m");
	}
	else{
		sprintf((char*)txtBuffer, "ADC %.2fV Battery",adc_float);
		dbg_println("\033[0;36mBattery State\033[0m");
	}
	ssd1306_WriteString(txtBuffer, Font_7x10, White);
	ssd1306_UpdateScreen();
#endif
#endif

	/** SECTION: GPS */
	state = 11;
#if(GPS_ENABLE)
#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 20);
	ssd1306_WriteString("GPS Measure...            ", Font_7x10, White);
	ssd1306_UpdateScreen();
#endif
	dbg_println("\033[1;36mBegin Get GNSS Value\033[0m");
	GetNormalizedGPS(&(_DATA.lat), &(_DATA.lng));
	//L80_ResetTrigger();
	HAL_IWDG_Refresh(&hiwdg);
#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 20);
	if(_DATA.lat == 999.9999999 || _DATA.lng == 999.9999999){
		sprintf((char*)txtBuffer, "GPS Fail %d          ",_STATUS.gps_fail);
		ssd1306_WriteString(txtBuffer, Font_7x10, White);
	}
	else{
		sprintf((char*)txtBuffer, "GPS %.2f,%.2f  ",_DATA.lat,_DATA.lng);
		ssd1306_WriteString(txtBuffer, Font_7x10, White);
	}
	ssd1306_UpdateScreen();
#endif
#endif

	/** SECTION: DENSO_RAM */
	state = 122;
#if(DENSORAM_ENABLE)
#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 30);
	ssd1306_WriteString("RAM P2 Measure...          ", Font_7x10, White);
	ssd1306_UpdateScreen();
#endif
//	HAL_Delay(1000);
	while(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_BUSY)){
		HAL_Delay(50);
	}
	DENSORAM_SendPageRequest(2);
	DENSORAM_RxITEnable();
	HAL_Delay(1250);
	DENSORAM_RxITDisable();
	dbg_println("\033[1;36mBegin Get RAM Page 2\033[0m");
	_STATUS.Flags.FlagRAM2_RetryTime = 0;
	_STATUS.Flags.FlagRAM2_ChecksumError = 0;
	for(int i = 0; i < RAM_RETRY; i++){
		DENSORAM_RxITEnable();
		for (int i = 0; i < RAM_TIMEOUT; i++) {
			if (_RAM.Flags.FlagDataReady == true) {
				DENSORAM_RxITDisable();
				break;
			}
			HAL_Delay(1);
		}
		if(DENSORAM_Checksum() == true){
			break;
		}
		else{
			_STATUS.Flags.FlagRAM2_ChecksumError = 1;
		}
		_STATUS.Flags.FlagRAM2_RetryTime++;
		DENSORAM_RxITDisable();
	}
	HAL_IWDG_Refresh(&hiwdg);

	_DATA.flag.FlagRAM2Error = _RAM.Flags.FlagOverrunError;
	_STATUS.Flags.FlagRAM2_FrameError = _RAM.Flags.FlagFrameError;
	_STATUS.Flags.FlagRAM2_NoiseError = _RAM.Flags.FlagNoiseError;

	for(int i = 0; i < 129; i++){
		_DATA.ram2[i] = _RAM.RxBuffer[i];
		sprintf(debugBuffer,"%02x,",_RAM.RxBuffer[i]);
		dbg_print(debugBuffer);
	}

	_DATA.ram2_length = _RAM.RAMIndex;
	sprintf((char*)debugBuffer,"Ram Data got %d bytes",_DATA.ram2_length);
	dbg_println(debugBuffer);

	state = 123;
#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 30);
	ssd1306_WriteString("RAM P3 Measure...       ", Font_7x10, White);
	ssd1306_UpdateScreen();
#endif
	while(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_BUSY)){
		HAL_Delay(50);
	}
	DENSORAM_SendPageRequest(3);
	DENSORAM_RxITEnable();
	HAL_Delay(1250);
	DENSORAM_RxITDisable();
	dbg_println("\033[1;36mBegin Get RAM Page 3\033[0m");
	_STATUS.Flags.FlagRAM3_RetryTime = 0;
	_STATUS.Flags.FlagRAM3_ChecksumError = 0;
	for(int i = 0; i < RAM_RETRY; i++){
		DENSORAM_RxITEnable();
		for (int i = 0; i < RAM_TIMEOUT; i++) {
			if (_RAM.Flags.FlagDataReady == true) {
				DENSORAM_RxITDisable();
				break;
			}
			HAL_Delay(1);
		}
		if(DENSORAM_Checksum() == true){
			break;
		}
		else{
			_STATUS.Flags.FlagRAM3_ChecksumError = 1;
		}
		_STATUS.Flags.FlagRAM3_RetryTime++;
		DENSORAM_RxITDisable();
	}
	HAL_IWDG_Refresh(&hiwdg);

	_DATA.flag.FlagRAM3Error = _RAM.Flags.FlagOverrunError;
	_STATUS.Flags.FlagRAM3_FrameError = _RAM.Flags.FlagFrameError;
	_STATUS.Flags.FlagRAM3_NoiseError = _RAM.Flags.FlagNoiseError;

	for(int i = 0; i < 129; i++){
		_DATA.ram3[i] = _RAM.RxBuffer[i];
		sprintf((char*)debugBuffer,"%02x,",_RAM.RxBuffer[i]);
		dbg_print(debugBuffer);
	}

	_DATA.ram3_length = _RAM.RAMIndex;
	sprintf((char*)debugBuffer,"Ram Data got %d bytes",_DATA.ram3_length);
	dbg_println(debugBuffer);

	state = 124;
#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 30);
	ssd1306_WriteString("RAM P4 Measure...         ", Font_7x10, White);
	ssd1306_UpdateScreen();
#endif
	DENSORAM_SendPageRequest(4);
	DENSORAM_RxITEnable();
	HAL_Delay(1250);
	DENSORAM_RxITDisable();
	dbg_println("\033[1;36mBegin Get RAM Page 4\033[0m");
	_STATUS.Flags.FlagRAM4_RetryTime = 0;
	_STATUS.Flags.FlagRAM4_ChecksumError = 0;
	for(int i = 0; i < RAM_RETRY; i++){
		DENSORAM_RxITEnable();
		for (int i = 0; i < RAM_TIMEOUT; i++) {
			if (_RAM.Flags.FlagDataReady == true) {
				DENSORAM_RxITDisable();
				break;
			}
			HAL_Delay(1);
		}
		if(DENSORAM_Checksum() == true){
			break;
		}
		else{
			_STATUS.Flags.FlagRAM4_ChecksumError = 1;
		}
		_STATUS.Flags.FlagRAM4_RetryTime++;
		DENSORAM_RxITDisable();
	}
	HAL_IWDG_Refresh(&hiwdg);
	_DATA.flag.FlagRAM4Error = _RAM.Flags.FlagOverrunError;

	for(int i = 0; i < 129; i++){
		_DATA.ram4[i] = _RAM.RxBuffer[i];
		sprintf(debugBuffer,"%02x,",_RAM.RxBuffer[i]);
		dbg_print(debugBuffer);
	}
	_DATA.ram4_length = _RAM.RAMIndex;
	sprintf((char*)debugBuffer,"Ram Data got %d bytes",_DATA.ram4_length);
	dbg_println(debugBuffer);
	DENSORAM_SendPageRequest(2);
	DENSORAM_RxITEnable();
	HAL_Delay(500);
	DENSORAM_RxITDisable();
#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 30);
	sprintf((char*)txtBuffer,"RAM %d, %d, %d      ",_DATA.ram2_length,_DATA.ram3_length,_DATA.ram4_length);
	ssd1306_WriteString(txtBuffer, Font_7x10, White);
	ssd1306_UpdateScreen();
#endif

#endif


	/** SECTION: UC200 */
		state = 13;
#if(UC200_ENABLE)
#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 40);
	ssd1306_WriteString("Begin to Send       ", Font_7x10, White);
	ssd1306_UpdateScreen();
#endif
	dbg_println("\033[1;36mBegin Present Publish.\033[0m");
	HAL_Delay(100);
		PackageData();
		if (u8check == false) {
			AddPackageToBackup();

		} else {
			if (UC200_MQTTPublish((uint8_t*) "iot/rammonitor1", _DATAString) == false) {
#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 40);
	ssd1306_WriteString("Send Fail.. Backup       ", Font_7x10, White);
	ssd1306_UpdateScreen();
#endif
				sprintf((char*)debugBuffer,"\033[1;31mPresent Publish Failed. Begin to Write Backup #%d\033[0m",save_index);
				dbg_println(debugBuffer);
				AddPackageToBackup();
			}
			else{
#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 40);
	ssd1306_WriteString("Send OK!      ", Font_7x10, White);
	ssd1306_UpdateScreen();
#endif
				dbg_println("\033[1;32mPresent Publish Success.\033[0m");
			}
		}
		HAL_IWDG_Refresh(&hiwdg);
		if (save_index > 0) {
#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 50);
	ssd1306_WriteString("Begin Resend..      ", Font_7x10, White);
	ssd1306_UpdateScreen();
#endif
			/* After save then resent */
			sprintf((char*)debugBuffer,"\033[1;36mBegin Backup Publish#%d\033[0m",save_index);
			dbg_println(debugBuffer);
			memset((char*)_DATAString,0,512);
			ReadBackup(save_index-1, _DATAString);
			dbg_print(_DATAString);
			if (UC200_MQTTPublish((uint8_t*) "iot/rammonitor", _DATAString) == true) {
				dbg_println("\033[1;33mBackup Publish Success, Decrease decrement\033[0m");
				RemovePackageFromBackup();
			} else {
				dbg_println("\033[1;31mBackup Publish Failed, Retain backup data\033[0m");
			}
		}


	u8check = UC200_RTCread(txtBuffer);
	u8check &= UC200_ConvertEpochTime((char*) txtBuffer, &(_DATA.timestamp));
	_STATUS.timestamp_end = _DATA.timestamp;
	_STATUS.operation_time = _STATUS.timestamp_end - _STATUS.timestamp_begin;
	sprintf((char*) debugBuffer, "\033[1;36mTotal time used is %lds\033[0m",(_STATUS.timestamp_end - _STATUS.timestamp_begin));
	dbg_println(debugBuffer);
	sprintf((char*) debugBuffer, "\033[1;36mFail Time is %d\033[0m",_STATUS.fail_time);
	dbg_println(debugBuffer);
	HAL_IWDG_Refresh(&hiwdg);
	if (_STATUS.fail_time > 5) {
		HAL_Delay(1);
		NVIC_SystemReset();
	}


	state = 14;
	wait_loop = SENDING_INTERVAL-(_STATUS.timestamp_end - _STATUS.timestamp_begin);
	wait_loop -= 5;
#if(OLED_ENABLE)
	ssd1306_SetCursor(0, 50);
	sprintf((char*)txtBuffer,"Time%d s. Fail %d",_STATUS.timestamp_end - _STATUS.timestamp_begin),_STATUS.fail_time;
	ssd1306_WriteString(txtBuffer, Font_7x10, White);
	ssd1306_UpdateScreen();
#endif

//	UC200_EnterSleepMode();
	if(wait_loop < 0){
		HAL_Delay(100);
	}
	else{
		if(wait_loop > SENDING_INTERVAL){
			wait_loop = SENDING_INTERVAL;
		}
		for(; wait_loop >= 0; wait_loop--){
//			u16check = UC200_TcpipReceiveAvailable();
//			if(u16check > 0){
//				u8check = UC200_MQTTgetSubscribeMessage(sub_room, txtBuffer, u16check);
//				if(u8check){
//					ssd1306_SetCursor(0, 50);
//					ssd1306_WriteString(txtBuffer, Font_7x10, White);
//					ssd1306_UpdateScreen();
//				}
//			}
			HAL_Delay(999);
			HAL_IWDG_Refresh(&hiwdg);
		}
	}
	_DATA.pack_no++;
#endif
}

int8_t I2C_Scan[128];

void I2C_Scanner(void){
	for (int i=1; i<128; i++)
	 	{
	 	  /*
	 	   * the HAL wants a left aligned i2c address
	 	   * &hi2c1 is the handle
	 	   * (uint16_t)(i<<1) is the i2c address left aligned
	 	   * retries 2
	 	   * timeout 2
	 	   */
	 	  HAL_StatusTypeDef result = HAL_I2C_IsDeviceReady(&hi2c2, (uint16_t)(i<<1), 2, 2);
	 	  if (result != HAL_OK) // HAL_ERROR or HAL_BUSY or HAL_TIMEOUT
	 	  {
	 		 I2C_Scan[i] = -1; // No ACK received at that address
	 	  }
	 	  if (result == HAL_OK)
	 	  {
	 		 I2C_Scan[i] = 1; // Received an ACK at that address
	 	  }
	 	}
}


