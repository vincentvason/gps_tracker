/*
 * proc_backup.c
 *
 *  Created on: Aug 25, 2020
 *      Author: Vason-PC
 */

#include "proc_backup.h"
#include <stdio.h>
#include <string.h>
#include "neth_uc200.h"



uint16_t save_limit = 0;
uint16_t save_index = 0;

bool AddPackageToBackup(){
	save_index++;
	if(save_limit < FILE_LIMIT) save_limit++;
	WRITE_SAVEINDEX(save_index);
	_STATUS.fail_time++;
	return WriteBackup(save_index, _DATAString);
}

bool RemovePackageFromBackup(){
	save_index--;
	save_limit--;
	WRITE_SAVEINDEX(save_index);
	if(_STATUS.fail_time > 0) _STATUS.fail_time--;
	if(save_limit <= 0){
		save_index = 0;
	}
	return true;
}


/**
 * [USER FUNCTION] Automatically backup your data
 * @param save_index Your save index
 * @param data Your data string
 * @return Process complete status.
 */
bool WriteBackup(int32_t save_index, uint8_t* data){
	bool check;
	uint32_t key;
	if(save_index <= 0){
		return false;
	}

	uint8_t filename[256] = {0};
	//Write String
	//If exceed file limit, delete oldest one.
	if(save_index > FILE_LIMIT){
		sprintf((char*)filename, "BACKUP%03ld", save_index-FILE_LIMIT);
		UC200_FileDelete(filename);
	}

	//Write to new file.
	sprintf((char*)filename,"BACKUP%03ld",save_index);
	UC200_FileNew(filename, data);
}

/**
 * [USER FUNCTION] Read backup your data .
 * @param save_index Your save index
 * @param data String that store read data
 * @return Process complete status.
 */
bool ReadBackup(int32_t save_index, uint8_t* data){
	uint8_t filename[256] = {0};
	//Read file from your back up
	if(save_index <= 0){
		return false;
	}

	uint32_t key;
	bool check;

	sprintf((char*)filename, "BACKUP%03ld", save_index);
	check = UC200_FileRead(filename, data, 1024);
	return check;
}


bool DeleteBackup(int32_t save_index){
	uint8_t filename[256] = {0};
	bool check;
	sprintf((char*)filename, "BACKUP%03ld", (save_index));
	check = UC200_FileDelete(filename);
	return check;
}


