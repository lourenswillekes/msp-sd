/*
 * sd_driver.h
 *
 *  Created on: Feb 2018
 *      Author: lourw
 */

#ifndef SD_DRIVER_H_
#define SD_DRIVER_H_

#include "driverlib.h"
#include "fatfs/src/ff.h"
#include "fatfs/src/diskio.h"
#include "spiDriver.h"


void sd_Init(void);
void sd_Open(char *fileName);
void sd_Write();
void sd_Close();


#endif /* SD_DRIVER_H_ */
