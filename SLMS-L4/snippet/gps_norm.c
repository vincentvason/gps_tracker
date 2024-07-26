#include <stdio.h>

#define GPS_LENGTH	5

void insertionSort(double arr[], int n);
void GetNormalizedGPS(double* lat, double* lng);

double input_lat[GPS_LENGTH] = {1.33133, 1.33344, 1.33255, 1.33566, 1.334777};
double input_lng[GPS_LENGTH] = {1.12123, 1.12534, 1.12445, 1.12256, 1.12367};

double gps_lat[GPS_LENGTH];
double gps_lng[GPS_LENGTH];

double final_lat, final_lng;

int main(void) {
  ///
  for(int i = 0;i < GPS_LENGTH; i++){
    printf("%f ",gps_lat[i]);
  }
  printf("\n");
  for(int i = 0;i < GPS_LENGTH; i++){
    printf("%f ",gps_lng[i]);
  }
  printf("\n\n");
  ///
  GetNormalizedGPS(&final_lat, &final_lng);

  printf("%f\n%f\n\n",final_lat,final_lng);
  return 0;
}

void GetNormalizedGPS(double* lat, double* lng){
	int filter_n = GPS_LENGTH;
	*lat = 0;
	*lng = 0;

	//SECTION: Latitude/Longitude Get
	for(int j = 0; j < GPS_LENGTH; j++){
		for (int i = 0; i <= 50; i++) {
			// if (GNSS_DataReady == true) {
			// 	break;
			// 	if(i == 50){
			// 		gps_lat[j] = 999.999999;
			// 		gps_lng[j] = 999.999999;
			// 	}
			// }
			// _DATA.lat = gnss_getLatitude();
			// _DATA.lng = gnss_getLongitude();
			// HAL_Delay(10);
      gps_lat[j] = input_lat[j];
      gps_lng[j] = input_lng[j];
		}
	}
	//SECTION: Latitude/Longitude Filter
	insertionSort(gps_lat,GPS_LENGTH);
	insertionSort(gps_lng,GPS_LENGTH);
  ///
  for(int i = 0;i < GPS_LENGTH; i++){
    printf("%f ",gps_lat[i]);
  }
  printf("\n");
  for(int i = 0;i < GPS_LENGTH; i++){
    printf("%f ",gps_lng[i]);
  }
  printf("\n\n");
  ///
	for(int i = 0;i <  GPS_LENGTH;i++){
		if((gps_lat[i] - gps_lat[GPS_LENGTH/2]) < -0.02){
			gps_lat[i] = 999.999999;
			gps_lng[i] = 999.999999;
		}
		if((gps_lat[i] - gps_lat[GPS_LENGTH/2]) > 0.02){
			gps_lat[i] = 999.999999;
			gps_lng[i] = 999.999999;
		}
		if((gps_lng[i] - gps_lng[GPS_LENGTH/2]) < -0.02){
			gps_lat[i] = 999.999999;
			gps_lng[i] = 999.999999;
		}
		if((gps_lng[i] - gps_lng[GPS_LENGTH/2]) > 0.02){
			gps_lat[i] = 999.999999;
			gps_lng[i] = 999.999999;
		}
	}
  ///
  for(int i = 0;i < GPS_LENGTH; i++){
    printf("%f ",gps_lat[i]);
  }
  printf("\n");
  for(int i = 0;i < GPS_LENGTH; i++){
    printf("%f ",gps_lng[i]);
  }
  printf("\n\n");
  ///
	insertionSort(gps_lat,GPS_LENGTH);
	insertionSort(gps_lng,GPS_LENGTH);

	for(int i = 0;i < GPS_LENGTH; i++){
		if(gps_lat[i] == 999.999999 || gps_lng[i] == 999.999999){
			filter_n = i;
			break;
		}
	}

  ///
  printf("%d ",filter_n);
  printf("\n\n");
  ///

	if(filter_n == 0){
		*lat = 999.999999;
		*lng = 999.999999;
		return;
	}
	else{
		for(int i = 0;i < filter_n; i++){
			*lat += gps_lat[i];
			*lng += gps_lng[i];
		}
		*lat /= filter_n;
		*lng /= filter_n;
		return;
	}
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

