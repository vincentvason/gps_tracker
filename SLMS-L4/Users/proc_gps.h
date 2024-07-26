/*
 * proc_gps.h
 *
 *  Created on: Aug 27, 2020
 *      Author: Vason-PC
 */

#ifndef PROC_GPS_H_
#define PROC_GPS_H_

#define GPS_SYSRESET 5

#include <stdbool.h>
#include "neth_gnss.h"
#include "neth_l80.h"
#include "proc.h"
#include "proc_data.h"

void GetNormalizedGPS(double* lat, double* lng);
void L80_ResetTrigger(void);

#endif /* PROC_GPS_H_ */
