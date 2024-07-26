/*
 * neth_l80.h
 *
 *  Created on: Sep 16, 2020
 *      Author: Vason-PC
 */

#ifndef NETH_L80_H_
#define NETH_L80_H_

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "usart.h"
#include "gpio.h"

#include "neth_gnss.h"

void L80_StandbyMode(bool enable);
void L80_BackupMode(bool enable);

#endif /* NETH_L80_H_ */
