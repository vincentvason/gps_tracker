/*
 * neth_l80.c
 *
 *  Created on: Sep 16, 2020
 *      Author: Vason-PC
 */


#include "neth_l80.h"

UART_HandleTypeDef* UartInst;

void L80_Init(UART_HandleTypeDef* huart){
	UartInst = huart;
	gnss_init(UartInst);
}

void L80_StandbyMode(bool enable){
	if(enable){
		HAL_UART_Transmit(UartInst, (uint8_t*)"$PMTK161,0*28\r\n", strlen("$PMTK161,0*28\r\n"), 1000);
	}
	else{
		HAL_UART_Transmit(UartInst, (uint8_t*)"A\r\n", strlen("$A\r\n"), 1000); //send any data will wake it up.
	}
}

void L80_BackupMode(bool enable){
	if(enable){
		HAL_GPIO_WritePin(GPS_PWR_CTRL_GPIO_Port, GPS_PWR_CTRL_Pin, GPIO_PIN_RESET);
		HAL_Delay(1);
		HAL_UART_Transmit(UartInst, (uint8_t*)"$PMTK225,4*2F\r\n", strlen("$PMTK225,4*2F\r\n"), 1000);
	}
	else{
		HAL_GPIO_WritePin(GPS_PWR_CTRL_GPIO_Port, GPS_PWR_CTRL_Pin, GPIO_PIN_SET);
	}
}

