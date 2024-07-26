/*
 * proc.h
 *
 *  Created on: Aug 25, 2020
 *      Author: Vason-PC
 */

#ifndef PROC_H_
#define PROC_H_

#include "adc.h"
#include "usart.h"
#include "rtc.h"
#include "iwdg.h"

#include "neth_uc200.h"
#include "neth_gnss.h"
#include "neth_l80.h"
#include "ssd1306.h"
#include "denso_ram.h"

#include "proc_data.h"
#include "proc_backup.h"
#include "proc_gps.h"

/* seconds */
#define SENDING_INTERVAL	 60
#define GPSRESET_INTERVAL	 60

void Setup(void);
void Loop(void);

#endif /* PROC_H_ */
