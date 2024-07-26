/*
 * proc_gps.c
 *
 *  Created on: Aug 27, 2020
 *      Author: Vason-PC
 */

#include "proc_gps.h"

#define GPS_LENGTH	7

/* Use neth_ATparser.h
#define DBG_UART		   huart1
#define dbg_print(...)     HAL_UART_Transmit(&DBG_UART, (uint8_t*)__VA_ARGS__, strlen((char*)__VA_ARGS__), 1000)
#define dbg_println(...)   HAL_UART_Transmit(&DBG_UART, (uint8_t*)__VA_ARGS__, strlen((char*)__VA_ARGS__), 1000);\
						   HAL_UART_Transmit(&DBG_UART, (uint8_t*)"\r\n", 2, 1000)
*/

double gps_lat[GPS_LENGTH];
double gps_lng[GPS_LENGTH];

void insertionSort(double arr[], int n);

void GetNormalizedGPS(double* lat, double* lng){
	int filter_n = GPS_LENGTH;
	int fail = 0;
	*lat = 0;
	*lng = 0;
	_DATA.flag.FlagGPSDetached = false;

//	L80_StandbyMode(false);
	//SECTION: Latitude/Longitude Get

	for(int j = 0; j < GPS_LENGTH; j++){
		gnss_RxITEnable();
		for (int i = 1; i <= 50; i++) {

			if (GNSS_DataReady == true) {
				break;
			}
			gps_lat[j] = gnss_getLatitude();
			gps_lng[j] = gnss_getLongitude();
			HAL_Delay(10);
		}
		if(gps_lat[j] == 9999.999999 || gps_lng[j] == 9999.999999) fail++;
		gnss_RxITDisable();
		gnss_reset();
		HAL_Delay(100);
	}

	if(fail >= GPS_LENGTH){
		*lat = 999.999999;
		*lng = 999.999999;
		_DATA.flag.FlagGPSDetached = true;
		_STATUS.gps_fail++;
		return;
	}


	//SECTION: Latitude/Longitude Filter
	insertionSort(gps_lat,GPS_LENGTH);
	insertionSort(gps_lng,GPS_LENGTH);

	for(int i = 0;i < GPS_LENGTH; i++){
		if(gps_lat[i] == 999.999999 || gps_lng[i] == 999.999999){
			filter_n = i;
			break;
		}
		if(gps_lat[i] == 9999.999999 || gps_lng[i] == 9999.999999){
			filter_n = i;
			break;
		}
	}

	for(int i = 0;i < filter_n;i++){
		if((gps_lat[i] - gps_lat[filter_n/2]) < -0.02){
			gps_lat[i] = 999.999999;
			gps_lng[i] = 999.999999;
		}
		if((gps_lat[i] - gps_lat[filter_n/2]) > 0.02){
			gps_lat[i] = 999.999999;
			gps_lng[i] = 999.999999;
		}
		if((gps_lng[i] - gps_lng[filter_n/2]) < -0.02){
			gps_lat[i] = 999.999999;
			gps_lng[i] = 999.999999;
		}
		if((gps_lng[i] - gps_lng[filter_n/2]) > 0.02){
			gps_lat[i] = 999.999999;
			gps_lng[i] = 999.999999;
		}
	}
	insertionSort(gps_lat,filter_n);
	insertionSort(gps_lng,filter_n);

	for(int i = 0;i < filter_n; i++){
		if(gps_lat[i] == 999.999999 || gps_lng[i] == 999.999999){
			filter_n = i;
			break;
		}
		if(gps_lat[i] == 9999.999999 || gps_lng[i] == 9999.999999){
			filter_n = i;
			break;
		}
	}

	if(filter_n == 0){
		*lat = 999.999999;
		*lng = 999.999999;
		_STATUS.gps_fail++;
		return;
	}
	else{
		for(int i = 0;i < filter_n; i++){
			*lat += gps_lat[i];
			*lng += gps_lng[i];
		}
		*lat /= filter_n;
		*lng /= filter_n;
		_STATUS.gps_fail = 0;
//		L80_StandbyMode(true);
		return;
	}

	gnss_reset();
}

void insertionSort(double arr[], int n)
{
    int i, j;
    double key;
    for (i = 1; i < n; i++)
    {
        key = arr[i];
        j = i - 1;

        /* Move elements of arr[0..i-1], that are
        greater than key, to one position ahead
        of their current position */
        while (j >= 0 && arr[j] > key)
        {
            arr[j + 1] = arr[j];
            j = j - 1;
        }
        arr[j + 1] = key;
    }
}

void L80_ResetTrigger(void){
	uint8_t gps_reset = (uint8_t)((float)GPSRESET_INTERVAL/(float)SENDING_INTERVAL);
	uint8_t system_reset = (uint8_t)((float)GPSRESET_INTERVAL*GPS_SYSRESET/(float)SENDING_INTERVAL);
	if(gps_reset < 1){ gps_reset = 1; }
	if(system_reset < 1){system_reset = 1; }

	if(_STATUS.gps_fail != 0 && (_STATUS.gps_fail % gps_reset == 0)){
		dbg_println("## GNSS Trigger - Begin GNSS Reset");
		gnss_hardwareReset();
	}
	if(_STATUS.gps_fail > system_reset){
		dbg_println("## GNSS Trigger - Begin System Reset");
		NVIC_SystemReset();
	}
}

