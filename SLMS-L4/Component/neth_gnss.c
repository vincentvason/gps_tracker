/*
 * neth_gnss.cc
 *
 *  Created on: Apr 15, 2020
 *      Author: Saifa K.
 */


#include "neth_gnss.h"

static UART_HandleTypeDef *UartInst = NULL;

/* Data change on interrupt --> Amplify by volatile */
bool GNSS_DataReady = false;
bool GNSS_DataError = false;
static GNSS_Data _Data;
static volatile uint8_t RxByte;
static volatile uint8_t DataBuffer[7][GNSS_BUFFER_LENGTH];
static volatile bool BufferReady;
static volatile uint8_t CurrentBuffer;


static volatile int comma[7][20] = {0};

#define GNSS_POWER_OFF()	HAL_GPIO_WritePin(GPS_PWR_CTRL_GPIO_Port, GPS_PWR_CTRL_Pin, GPIO_PIN_RESET)
#define GNSS_POWER_ON()	    HAL_GPIO_WritePin(GPS_PWR_CTRL_GPIO_Port, GPS_PWR_CTRL_Pin, GPIO_PIN_SET)
#define GNSS_RST_OFF()      HAL_GPIO_WritePin(GPS_RESET_GPIO_Port, GPS_RESET_Pin, GPIO_PIN_RESET);
#define GNSS_RST_ON()       HAL_GPIO_WritePin(GPS_RESET_GPIO_Port, GPS_RESET_Pin, GPIO_PIN_SET);

void gnss_hardwareReset(void){
	GNSS_POWER_ON();
	GNSS_RST_OFF();
	HAL_Delay(1);

	GNSS_RST_ON();
	HAL_Delay(12); /* must more than 10ms */
	GNSS_RST_OFF();
}

void gnss_hardwarePowerOff(void){
	GNSS_POWER_OFF();
}

/**********************************************/
/* NETH_GNSS class function                   */
/**********************************************/
bool gnss_init(UART_HandleTypeDef *huart)
{
	UartInst = huart;

	if(UartInst == NULL){
		return false;
	}

	memset((char*)DataBuffer[0], 0, GNSS_BUFFER_LENGTH);
	memset((char*)DataBuffer[1], 0, GNSS_BUFFER_LENGTH);
	memset((char*)DataBuffer[2], 0, GNSS_BUFFER_LENGTH);
	memset((char*)DataBuffer[3], 0, GNSS_BUFFER_LENGTH);
	memset((char*)DataBuffer[4], 0, GNSS_BUFFER_LENGTH);
	memset((char*)DataBuffer[5], 0, GNSS_BUFFER_LENGTH);
	memset((char*)DataBuffer[6], 0, GNSS_BUFFER_LENGTH);
	BufferReady = false;
	CurrentBuffer = 0;

	/* Power-off then ON */
	gnss_hardwareReset();

	HAL_UART_Abort_IT(UartInst);
	HAL_Delay(10);
	__HAL_UART_ENABLE_IT(UartInst, UART_IT_RXNE);
	__HAL_UART_ENABLE_IT(UartInst, UART_IT_IDLE);

	//command
	HAL_UART_Transmit(UartInst, (uint8_t*)"$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n", strlen("$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n"), 1000);
	_Data.latitude = 9999.999999;
	_Data.longitude = 9999.999999;

	return true;
}

void gnss_reset(void){
	_Data.latitude = 9999.999999;
	_Data.longitude = 9999.999999;
}

void gnss_updatePosition(void)
{
	memset((char*)DataBuffer[0], 0, GNSS_BUFFER_LENGTH);
	memset((char*)DataBuffer[1], 0, GNSS_BUFFER_LENGTH);
	memset((char*)DataBuffer[2], 0, GNSS_BUFFER_LENGTH);
	memset((char*)DataBuffer[3], 0, GNSS_BUFFER_LENGTH);
	memset((char*)DataBuffer[4], 0, GNSS_BUFFER_LENGTH);
	memset((char*)DataBuffer[5], 0, GNSS_BUFFER_LENGTH);
	memset((char*)DataBuffer[6], 0, GNSS_BUFFER_LENGTH);
	BufferReady = false;
	CurrentBuffer = 0;
}



void gnss_RxCallback(void)
{
	/* If received new data byte */
	if(__HAL_UART_GET_IT(UartInst,UART_IT_RXNE)){
		RxByte = (uint8_t)(UartInst->Instance->RDR);
#if(GNSS_ECHO_MODE)
		GNSS_DBG_UART.Instance->TDR = RxByte;
#endif
		gnss_chartoBuffer(RxByte);
	}

	/* Idle flag set, all bytes received */
	if(__HAL_UART_GET_IT(UartInst, UART_IT_IDLE)){
		__HAL_UART_CLEAR_IDLEFLAG(UartInst);

		gnss_ExtractData();
		gnss_updatePosition();
		GNSS_DataError = __HAL_UART_GET_FLAG(UartInst, UART_FLAG_ORE);
		if(GNSS_DataError){
			HAL_UART_Abort_IT(UartInst);
			HAL_Delay(10);
			__HAL_UART_DISABLE_IT(UartInst, UART_IT_RXNE);
			__HAL_UART_DISABLE_IT(UartInst, UART_IT_IDLE);
			HAL_UART_Abort_IT(UartInst);
			HAL_Delay(10);
			__HAL_UART_ENABLE_IT(UartInst, UART_IT_RXNE);
			__HAL_UART_ENABLE_IT(UartInst, UART_IT_IDLE);
		}

		_Data.IdleCount++;
		if(_Data.IdleCount >= 1 && GNSS_DataError == false){
			GNSS_DataReady = true;
		}
		else if(_Data.IdleCount >= 5){
			GNSS_DataReady = true;
		}
	}
}

void gnss_RxITEnable(void){
	HAL_UART_Abort_IT(UartInst);
	HAL_Delay(10);
	__HAL_UART_ENABLE_IT(UartInst, UART_IT_RXNE);
	__HAL_UART_ENABLE_IT(UartInst, UART_IT_IDLE);
	_Data.IdleCount = 0;
	GNSS_DataReady = false;
}

void gnss_RxITDisable(void){
	HAL_UART_Abort_IT(UartInst);
	HAL_Delay(10);
	__HAL_UART_DISABLE_IT(UartInst, UART_IT_RXNE);
	__HAL_UART_DISABLE_IT(UartInst, UART_IT_IDLE);
}



void gnss_chartoBuffer(char rx_data)
{
	static volatile int i=0, j=0; /* i for index, j for commas position */

	if (rx_data == '$')
	{
		i=0;
		j=0;
	}

	DataBuffer[CurrentBuffer][i] = rx_data;

	if (rx_data == ',')
	{
		comma[CurrentBuffer][j]=i;
		j++;
		if (j > 19)
		{
			j = 19;
		}
	}

	i++;

	if (j != 0)
	{
		if('\n' != rx_data)
		{
			DataBuffer[CurrentBuffer][i-1] = rx_data;
		}
		else
		{
			if(true == gnss_ChecksumVerify((uint8_t*)DataBuffer))
			{
				CurrentBuffer++;
			}
		}
	}
}


void gnss_ExtractData(void)
{
	double tempLat, tempLng;
	double degree, second;
	double lat, lng;
//	uint8_t nmeaType[7] = {0};

	/* Detect NMEA of each buffer */
	for(int i =0;i<7;i++)
	{
	   if((DataBuffer[i][4] == 'M') && (DataBuffer[i][5] == 'C'))
	   {
			/* Fixed status V = void, A = active */
			_Data.fixedStatus = DataBuffer[i][comma[i][1]+1];

			/* Date */
			_Data.date = atoi((char*)&DataBuffer[i][comma[i][8]+1]);

			/* UTC time*/
			_Data.utcTime = atoi((char*)&DataBuffer[i][comma[i][0]+1]);

			/* Latitude/Longitude */
			if('A' == _Data.fixedStatus)
			{
				/* Due to incorrect value when use %f received data, So, will integer instead */
				tempLat = strtod((char*)&DataBuffer[i][comma[i][2]+1],NULL);
				tempLng = strtod((char*)&DataBuffer[i][comma[i][4]+1],NULL);

				//sscanf((char*)&DataBuffer[i][comma[i][2]+1], "%lf", &tempLat);
				//sscanf((char*)&DataBuffer[i][comma[i][4]+1], "%lf", &tempLng);

				/* Latitude calculation */
				degree = (int)(tempLat / 100.0);
				second = tempLat - (degree * 100.0);
				lat = degree + (second / 60.0);
				if(DataBuffer[i][comma[i][3]+1] == 'S')
				{
					lat = -lat;
				}
				_Data.latitude = (degree + (second / 60.0));

				/* Latitude calculation */
				degree = (int)(tempLng / 100.0);
				second = tempLng - (degree * 100.0);
				lng = degree + (second / 60.0);
				if(DataBuffer[i][comma[i][5]+1] == 'W')
				{
					lng = -lng;
				}
				_Data.longitude = lng;
			}
			else
			{
				_Data.latitude  = 999.999999;
				_Data.longitude = 999.999999;
			}
	   }
	   else if((DataBuffer[i][4] == 'T') && (DataBuffer[i][5] == 'G'))
	   {
		   _Data.speed_N = atof((char*)&DataBuffer[i][comma[i][4]+1]);
		   _Data.speed_k = atof((char*)&DataBuffer[i][comma[i][6]+1]);
	   }
	   else if((DataBuffer[i][4] == 'G') && (DataBuffer[i][5] == 'A'))
	   {

		   _Data.fixQuality    = atoi((char*)&DataBuffer[i][comma[i][5]+1]);
		   _Data.numSatellites = atoi((char*)&DataBuffer[i][comma[i][6]+1]);
		   _Data.altitude      = atof((char*)&DataBuffer[i][comma[i][8]+1]);
		   _Data.height        = atof((char*)&DataBuffer[i][comma[i][10]+1]);
	   }
	   else if((DataBuffer[i][4] == 'S') && (DataBuffer[i][5] == 'A'))
	   {

			_Data.PDOP = atof((char*)&DataBuffer[i][comma[i][14]+1]);
			_Data.HDOP = atof((char*)&DataBuffer[i][comma[i][15]+1]);
			_Data.VDOP = atof((char*)&DataBuffer[i][comma[i][16]+1]);
	   }
	   else if((DataBuffer[i][4] == 'S') && (DataBuffer[i][5] == 'V'))
	   {

	   }
	   else if((DataBuffer[i][4] == 'L') && (DataBuffer[i][5] == 'L'))
	   {

	   }
	   else if((DataBuffer[i][4] == 'X') && (DataBuffer[i][5] == 'T'))
	   {

	   }
	}
}

double gnss_getLatitude(void)
{
	return _Data.latitude;
}

double gnss_getLongitude(void)
{
	return _Data.longitude;
}

char gnss_getFixedStatus(void){
	return _Data.fixedStatus;
}

int gnss_getDate(void){
	return _Data.date;
}

int gnss_getUTCtime(void){
	return _Data.utcTime;
}

int gnss_getFixedQuality(void){
	return _Data.fixQuality;
}

int gnss_getNumofSatellite(void){
	return _Data.numSatellites;
}

float gnss_getAltitude(void){
	return _Data.altitude;
}

float gnss_getHeight(void){
	return _Data.height;
}

float gnss_getPDOP(void){
	return _Data.PDOP;
}

float gnss_getHDOP(void){
	return _Data.HDOP;
}

float gnss_getVDOP(void){
	return _Data.VDOP;
}

float gnss_getSpeedOverGround(uint8_t unit){
	if(SPEED_KNOT)
		return _Data.speed_N;
	else
		return _Data.speed_k;
}

bool gnss_ChecksumVerify(uint8_t *Data)
{
	int checksumCal = 0;
	int checksumGPS = 0;

	uint8_t *ptr = Data;

	/* Data must valid and first character must be '$' character */
	if((NULL == ptr) || ('$' != *ptr))
	{
		return 0;
	}

	/* Shift position to first of calculate position */
	ptr++;

	/* Calculate until found '*' character */
	do
	{
		checksumCal = checksumCal ^ (*ptr++);
	}while(((*ptr) != '*') && ((*ptr) != 0));

	/* Update position then read value from GPS syntax */
	ptr++;
	sscanf((char*)ptr, "%x" , &checksumGPS);

	if(checksumGPS == checksumCal)
	{
		return CHECKSUM_OK;
	}

	return CHECKSUM_NG;
}

#if(0)
/* Checksum prototype */
uint8_t ChecksumCalculate(uint8_t *Data)
{
	uint8_t CheckSum = 0;

	uint8_t *ptr = Data;

	/* Data must valid and first character must be '$' character */
	if((NULL == ptr) || ('$' != *ptr))
	{
		return 0;
	}

	/* Shift position to first of calculate position */
	ptr++;

	/* Calculate until found '*' character */
	do
	{
		CheckSum = CheckSum ^ (*ptr++);
	}while(((*ptr) != '*') && ((*ptr) != 0));

	return CheckSum;
}
#endif


