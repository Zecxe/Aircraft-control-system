#ifndef __I2C_DRIVER_H
#define __I2C_DRIVER_H

#include "stdint.h"

#define CB_MEM_ADDRESS			0x50
#define CB_MEM_ADDRESS_LENGTH	2

//struct CB_MEM{
//	#define CB_MEM_ADDRESS 0x50
//	
//};

int init_i2c_driver(void);
int mem_wr(uint8_t chipAddr, uint16_t startAddr, uint8_t *dataBuf,
			uint8_t length);
int mem_rd(uint8_t chipAddr, uint16_t startAddr, uint8_t *dataBuf,
			uint8_t length);

#endif