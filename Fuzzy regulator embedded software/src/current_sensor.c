#include "MDR32FxQI_ssp.h"
#include "MDR32FxQI_rst_clk.h"
#include "MDR32FxQI_port.h"
#include "current_sensor.h"

int ssp_init()
{
        PORT_InitTypeDef sPortInit;
	SSP_InitTypeDef  sSSPInit;

        // ----------------- SSP PINS CONFIGURATION -----------------
        RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTF, ENABLE);
        
        PORT_StructInit(&sPortInit);
        sPortInit.PORT_Pin		= PORT_Pin_0 | PORT_Pin_1;
        sPortInit.PORT_FUNC		= PORT_FUNC_ALTER;
        sPortInit.PORT_MODE		= PORT_MODE_DIGITAL;
        sPortInit.PORT_PULL_UP		= PORT_PULL_UP_OFF;
        sPortInit.PORT_PULL_DOWN	= PORT_PULL_DOWN_OFF;
	sPortInit.PORT_SPEED		= PORT_SPEED_FAST;
	sPortInit.PORT_OE		= PORT_OE_OUT;
	PORT_Init(MDR_PORTF, &sPortInit);

	sPortInit.PORT_Pin		= PORT_Pin_3;
	sPortInit.PORT_OE		= PORT_OE_IN;
	PORT_Init(MDR_PORTF, &sPortInit);

	sPortInit.PORT_Pin		= PORT_Pin_2;
	sPortInit.PORT_FUNC		= PORT_FUNC_PORT;
	sPortInit.PORT_OE		= PORT_OE_OUT;
	PORT_Init(MDR_PORTF, &sPortInit);
	// ----------------- EBD OF SSP PINS CONFIGURATION -----------------

	// ----------------- SSP INTERFACE CONFIGURATION -----------------
	SSP_DeInit(MDR_SSP1);
	SSP_BRGInit(MDR_SSP1, SSP_HCLKdiv8);

	SSP_StructInit(&sSSPInit);
	sSSPInit.SSP_SCR		= 1;
	sSSPInit.SSP_CPSDVSR		= 5;
	sSSPInit.SSP_Mode		= SSP_ModeMaster;
	sSSPInit.SSP_WordLength		= SSP_WordLength8b;
	sSSPInit.SSP_SPH		= SSP_SPH_1Edge;
	sSSPInit.SSP_SPO		= SSP_SPO_Low;
	sSSPInit.SSP_FRF		= SSP_FRF_SPI_Motorola;
	sSSPInit.SSP_HardwareFlowControl= SSP_HardwareFlowControl_None;
	SSP_Init(MDR_SSP1, &sSSPInit);
	SSP_Cmd(MDR_SSP1, ENABLE);
	// ----------------- END OF SSP INTERFACE CONFIGURATION ----------------- 

	return 0;
}

uint8_t current_sensor_read_ID(enum current_sensor sensor)
{
	if (sensor == SENSOR_1)
		MDR_PORTF->RXTX &= ~PORT_Pin_2;
	else
		MDR_PORTF->RXTX |= PORT_Pin_2;

	uint16_t command = ((RREG | ID) << 8) | 0x00;
	SSP_SendData(MDR_SSP1, command);
	while (SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_TFE) == RESET);
	uint16_t dummy = NOP;
	SSP_SendData(MDR_SSP1, dummy);
	while (SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_RNE) == RESET);
	return 	(uint8_t)(SSP_ReceiveData(MDR_SSP1) >> 4);
}

void current_sensor_set_80SPS(enum current_sensor sensor)
{
	if (sensor == SENSOR_1)
		MDR_PORTF->RXTX &= ~PORT_Pin_2;
	else
		MDR_PORTF->RXTX |= PORT_Pin_2;

	uint16_t command = ((WREG | SYS0) << 8) | 0x00;
	SSP_SendData(MDR_SSP1, command);
	while (SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_TFE) == RESET);
	command = 0x04;
	SSP_SendData(MDR_SSP1, command);
	while (SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_RNE) == RESET);
}

int32_t current_sensor_get_current(enum current_sensor sensor)
{
	int32_t res;

	if (sensor == SENSOR_1)
		MDR_PORTF->RXTX &= ~PORT_Pin_2;
	else
		MDR_PORTF->RXTX |= PORT_Pin_2;

	uint16_t command = (RDATA << 8) | NOP;
	SSP_SendData(MDR_SSP1, command);
	while (SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_TFE) == RESET);
	command = (NOP << 8) | NOP;
	while (SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_RNE) == RESET);
	res = SSP_ReceiveData(MDR_SSP1) << 16;
	SSP_SendData(MDR_SSP1, command);
	while (SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_TFE) == RESET);
	while (SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_RNE) == RESET);
	res |= SSP_ReceiveData(MDR_SSP1);
	
	return res;
}