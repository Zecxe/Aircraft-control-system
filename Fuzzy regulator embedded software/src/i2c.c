/**
  * FILE i2c_driver.c
  */

#include "MDR32FxQI_i2c.h"
#include "MDR32FxQI_port.h"
#include "MDR32FxQI_rst_clk.h"
#include "i2c.h"

// TO-DO:
//			1. Add check of configured SCL speed if it is possible
//			2. Do not ovewrite values of PORT registers. Read them first, then write
//			3. Make a retval in init_i2c_driver
//			4. Add memory address incremental when writing data


/**
  * @brief  Initializes I2C hardware and associated PORTS.
  * @param  None.
  * @note:  See TO-DO in file
  * @retval None.
  */
int init_i2c_driver()
{
	// Create structs
	PORT_InitTypeDef port;
	I2C_InitTypeDef i2c;
	
	// Init CLK for PORT and I2C
	RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTC, ENABLE);
	RST_CLK_PCLKcmd(RST_CLK_PCLK_I2C, ENABLE);
	
	// Configure ports
	PORT_StructInit(&port);
	port.PORT_FUNC = PORT_FUNC_ALTER;
	port.PORT_MODE = PORT_MODE_DIGITAL;
	port.PORT_PULL_UP = PORT_PULL_UP_OFF;
	port.PORT_PULL_DOWN = PORT_PULL_DOWN_OFF;
	port.PORT_SPEED = PORT_SPEED_FAST;
	port.PORT_Pin = PORT_Pin_0 | PORT_Pin_1;
	//port.PORT_OE = PORT_OE_OUT;
	PORT_Init(MDR_PORTC, &port);
	
	I2C_StructInit(&i2c);
	i2c.I2C_ClkDiv = 3;
	i2c.I2C_Speed = I2C_SPEED_UP_TO_400KHz;
	I2C_Init(&i2c);
	
	I2C_Cmd(ENABLE);
	
	return 0;
}

/**
  * @brief  Writes data array to I2C memory chip.
  * @param  chipAddr_address: specifies address of memory chip
  * @param  startAddr: specifies start address of block in memory chip
  * @param  dataBuf: pointer to output data buffer
  * @param  length: number of data bytes to write
  * @retval None.
  */
int mem_wr(uint8_t chipAddr, uint16_t startAddr, uint8_t *dataBuf,
			uint8_t length)
{
	int res;
	int i;
	
	uint8_t slaveAddr = chipAddr << 1;
	uint8_t *startAddr_H = (uint8_t*)&startAddr + 1;

	//  THIS LINE MUST BE DELETED AFTER ADDING AUTO INCREMENTAL OF ADDRESS
	if (length >= 32)
		return -1;
	
	// Establish communication
	I2C_Send7bitAddress(slaveAddr, I2C_Direction_Transmitter);
	do {
		res = I2C_CheckEvent(I2C_EVENT_TRANSFER_IN_PROGRESS);
	} while (res == SUCCESS);
	res = I2C_CheckEvent(I2C_EVENT_ACK_FOUND);
	if (res == ERROR) return -1;
	
	// Send block address
	for (i = 0; i < CB_MEM_ADDRESS_LENGTH; i++) {
		I2C_SendByte(*(startAddr_H - i));
		do {
			res = I2C_CheckEvent(I2C_EVENT_TRANSFER_IN_PROGRESS);
		} while (res == SUCCESS);
		res = I2C_CheckEvent(I2C_EVENT_ACK_FOUND);
		if (res == ERROR) return -1;
	}
	
	// Write block of data
	for (i = 0; i < length; i++) {
		I2C_SendByte(*(dataBuf + i));
		do {
			res = I2C_CheckEvent(I2C_EVENT_TRANSFER_IN_PROGRESS);
		} while (res == SUCCESS);
		res = I2C_CheckEvent(I2C_EVENT_ACK_FOUND);
		if (res == ERROR) return -1;
	}
	
	//  Close communication
	I2C_SendSTOP();
	
	return 0;
}

/**
  * @brief  Reads data array from i2c memory chip on control board.
  * @param  chipAddr_address: specifies address of memory chip
  * @param  startAddr: specifies start address of block in memory chip
  * @param  dataBuf: pointer to output data buffer
  * @param  length: number of data bytes to read
  * @retval None.
  */
int mem_rd(uint8_t chipAddr, uint16_t startAddr, uint8_t *dataBuf,
			uint8_t length)
{
	return 0;
	
}
