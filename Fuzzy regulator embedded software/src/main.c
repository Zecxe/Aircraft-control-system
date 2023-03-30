#include "MDR32FxQI_config.h"
#include "MDR32FxQI_port.h"
#include "MDR32FxQI_uart.h"
#include "MDR32FxQI_rst_clk.h"
#include "MDR32FxQI_i2c.h"
#include "i2c.h"
#include "clock.h"
#include "main.h"
#include "mb.h"
#include "mbregmap.h"
#include "fxd_pnt_math.h"
#include "fuzzy_regulator.h"
#include "motor_control.h"
#include "current_sensor.h"

#include "string.h"

int state = 0;

/*------------- Static MODBUS variables -------------*/
// 	CREATE DIFFERENT BUFS FOR DIFFERENT FUZZY REGULATORS
static uint16_t	usRegHoldingStart = REG_HOLDING_START;
static int32_t	lRegHoldingBuf[REG_HOLDING_NREGS] = {0};

static USHORT	usRegCoilsStart = REG_COILS_START;
static BOOL	bRegCoilsBuf[REG_COILS_NREGS] = {0};

// Fuzzy regulator struct
struct __fuzzy_regulator fuzzyRegulator;

eMBErrorCode
eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
	return MB_ENOREG;
	/*
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;

    if( ( usAddress >= REG_INPUT_START )
        && ( usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS ) )
    {
        iRegIndex = ( int )( usAddress - usRegInputStart );
        while( usNRegs > 0 )
        {
            *pucRegBuffer++ =
                ( unsigned char )( usRegInputBuf[iRegIndex] >> 8 );
            *pucRegBuffer++ =
                ( unsigned char )( usRegInputBuf[iRegIndex] & 0xFF );
            iRegIndex++;
            usNRegs--;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
	

    return eStatus;
	*/
}


eMBErrorCode
eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs,
                 eMBRegisterMode eMode )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;
	SHORT			*lpRegister;

   	if( ( usAddress >= REG_HOLDING_START )
       		&& ( usAddress + usNRegs <= REG_HOLDING_START + REG_HOLDING_NREGS ) )
   	{
		switch (eMode)
		{
			case MB_REG_READ:
				iRegIndex = ( int )( usAddress - usRegHoldingStart );
				lpRegister = (SHORT *)&lRegHoldingBuf[iRegIndex];
       			while( usNRegs > 0 )
        		{
					memcpy(pucRegBuffer, lpRegister, sizeof(*lpRegister));
					pucRegBuffer += sizeof(*lpRegister);
					lpRegister++;
            				usNRegs--;
        		}
				break;
			case MB_REG_WRITE:
				iRegIndex = ( int )( usAddress - usRegHoldingStart );
				lpRegister = (SHORT *)&lRegHoldingBuf[iRegIndex];
        		while( usNRegs > 0 )
        		{
					memcpy(lpRegister, pucRegBuffer, sizeof(*lpRegister));
					pucRegBuffer += sizeof(*lpRegister);
					lpRegister++;
            				usNRegs--;
        		}
			default:
				break;
    	}
	}
    else
    {
       	eStatus = MB_ENOREG;
    }

    return eStatus;
}

// Fix coils finction
eMBErrorCode
eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils,
               eMBRegisterMode eMode )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;

	if (eMode == MB_REG_READ) 
	{
		if( ( usAddress >= REG_COILS_START )
        		&& ( usAddress + usNCoils <= REG_COILS_START + REG_COILS_NREGS ) )
    	{
        	iRegIndex = ( int )( usAddress - usRegCoilsStart );
        	while( usNCoils > 0 )
        	{
            	*pucRegBuffer++ = ( unsigned char )bRegCoilsBuf[iRegIndex];
            	iRegIndex++;
            	usNCoils--;
        	}
    	}
    	else
    	{
        	eStatus = MB_ENOREG;
    	}
	}
	else
	{
		if( ( usAddress >= REG_COILS_START )
        		&& ( usAddress + usNCoils <= REG_COILS_START + REG_COILS_NREGS ) )
    	{
        	iRegIndex = ( int )( usAddress - usRegCoilsStart );
        	while( usNCoils > 0 )
        	{
            	bRegCoilsBuf[iRegIndex] = *pucRegBuffer++;
            	iRegIndex++;
            	usNCoils--;
        	}
    	}
    	else
    	{
        	eStatus = MB_ENOREG;
    	}
	}

    return eStatus;
}

eMBErrorCode
eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
    return MB_ENOREG;
}

int main()
{
	// Init external clock and set to freq = 80 MHz
	config_ext_clk();

	// Init TIM/CNT1 for step calculation
	//step_timer_init();

	init_fuzzy_regulator(lRegHoldingBuf, bRegCoilsBuf, &fuzzyRegulator);
	//fuzzy_reg_transform(fuzzyRegulator);
	//fuzzy_regulator_update_step(&fuzzyRegulator);
	
	//motor_control_init();
	//motor_control_write_mode(DECAY, MOTOR1);
	//motor_control_write_state(RUN, MOTOR1);
	//set_main_motor_out_volt(10 * 1024, CLOCKWISE);
	//set_sub_motor_out_volt(10 * 1024, CLOCKWISE); - TDO ERROR WHILE USING IT

	eMBErrorCode    eStatus;
	// Настройки UART не поддерживаются кроме скорости, необходимо 
	// в portserial.c в xMBPortSerialInit описать выбор режимов

	eStatus = eMBInit( MB_RTU, 0x01, 0, 115200, MB_PAR_NONE ); 


        /* Enable the Modbus Protocol Stack. */
	eStatus = eMBEnable(  );

	while(1)
	{
		volatile uint8_t tmp = *fuzzyRegulator.REG_ON;
		volatile uint8_t tmp2 = *fuzzyRegulator.STEP_CMPLT;
		
		if ((*fuzzyRegulator.REG_ON == TRUE) && (*fuzzyRegulator.STEP_CMPLT == FALSE))
		{
			fuzzy_reg_transform(fuzzyRegulator);
		}
		
		eStatus = eMBPoll(  );
        	//обработчик ошибок
        	if (eStatus!= MB_ENOREG){};

        /* Here we simply count the number of poll cycles. */
        
	}
    	return 0;
}