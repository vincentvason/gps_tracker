/*
 * proc_backup.h
 *
 *  Created on: Aug 25, 2020
 *      Author: Vason-PC
 */

#ifndef PROC_BACKUP_H_
#define PROC_BACKUP_H_


#include "proc_data.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "neth_uc200.h"
#include "rtc.h"

#define FILE_LIMIT	300

#define	WRITE_SAVEINDEX(x)		HAL_RTCEx_BKUPWrite(&hrtc, 0, (uint32_t)x)
#define READ_SAVEINDEX()		HAL_RTCEx_BKUPRead(&hrtc, 0)

extern uint16_t save_index;
extern uint16_t save_limit;

bool WriteBackup(int32_t save_index, uint8_t* data);
bool ReadBackup(int32_t save_index, uint8_t* data);
bool DeleteBackup(int32_t save_index);
bool AddPackageToBackup(void);
bool RemovePackageFromBackup(void);

#endif /* PROC_BACKUP_H_ */
