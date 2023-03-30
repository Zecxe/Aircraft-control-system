/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id$
 */

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- Platform specific includes -----------------------*/
#include "MDR32FxQI_rst_clk.h"
#include "MDR32FxQI_timer.h"
#include "MDR32FxQI_port.h"

/* ----------------------- static functions ---------------------------------*/
static void prvvTIMERExpiredISR( void );

/* ----------------------- Start implementation -----------------------------*/

static void initDebugLED(void)
{
	PORT_InitTypeDef port;

	RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTA, ENABLE);
	PORT_StructInit(&port);
	
	port.PORT_Pin = PORT_Pin_2;
	port.PORT_FUNC = PORT_FUNC_PORT;
	port.PORT_MODE = PORT_MODE_DIGITAL;
	port.PORT_OE = PORT_OE_OUT;
	port.PORT_PULL_UP = PORT_PULL_UP_OFF;
	port.PORT_PULL_DOWN = PORT_PULL_DOWN_OFF;
	port.PORT_SPEED = PORT_SPEED_FAST;
	PORT_Init(MDR_PORTA, &port);

	PORT_ResetBits(MDR_PORTA, PORT_Pin_2);

	return;
}

BOOL
xMBPortTimersInit( USHORT usTim1Timerout50us )
{
    TIMER_CntInitTypeDef sTIM2CntInit;
    TIMER_DeInit(MDR_TIMER2);

    // Enable clocking for TIMER2 perihireal
    RST_CLK_PCLKcmd(RST_CLK_PCLK_TIMER2, ENABLE);

    // Configure TIMER2 settings for period 50us
    sTIM2CntInit.TIMER_Prescaler = 499;
    sTIM2CntInit.TIMER_IniCounter = 0;
    sTIM2CntInit.TIMER_Period = usTim1Timerout50us;
    sTIM2CntInit.TIMER_CounterMode = TIMER_CntMode_ClkFixedDir;
    sTIM2CntInit.TIMER_CounterDirection = TIMER_CntDir_Up;
    sTIM2CntInit.TIMER_EventSource = TIMER_EvSrc_TIM_CLK;
    sTIM2CntInit.TIMER_FilterSampling = TIMER_FDTS_TIMER_CLK_div_1;
    sTIM2CntInit.TIMER_ARR_UpdateMode = TIMER_ARR_Update_Immediately;
    sTIM2CntInit.TIMER_ETR_FilterConf = TIMER_Filter_1FF_at_TIMER_CLK;
    sTIM2CntInit.TIMER_ETR_Prescaler = TIMER_ETR_Prescaler_None;
    sTIM2CntInit.TIMER_ETR_Polarity = TIMER_ETRPolarity_NonInverted;
    sTIM2CntInit.TIMER_BRK_Polarity = TIMER_BRKPolarity_NonInverted;
    TIMER_CntInit(MDR_TIMER2, &sTIM2CntInit);

    // Enable TIM2 interrupt when CNT == ARR
    TIMER_ITConfig(MDR_TIMER2, TIMER_STATUS_CNT_ARR, ENABLE);
    //MDR_TIMER2->STATUS &= ~TIMER_IE_CNT_ARR_EVENT_IE;

    // Globally init interrupt vector
    NVIC_EnableIRQ(Timer2_IRQn);

    // Set TIM/CNT2 clock from HCLK with prescaler of 4
    TIMER_BRGInit(MDR_TIMER2, TIMER_HCLKdiv8);

    // Enable TII/CNT2
    //TIMER_Cmd(MDR_TIMER2, ENABLE);

    return TRUE;
}

//inline 
void
vMBPortTimersEnable(  )
{
    /* Enable the timer with the timeout passed to xMBPortTimersInit( ) */
    TIMER_Cmd(MDR_TIMER2, DISABLE);
    TIMER_SetCounter(MDR_TIMER2, 0);
    TIMER_Cmd(MDR_TIMER2, ENABLE);
}

//inline 
void
vMBPortTimersDisable(  )
{
    /* Disable any pending timers. */
    TIMER_Cmd(MDR_TIMER2, DISABLE);
    TIMER_SetCounter(MDR_TIMER2, 0);
}

/* Create an ISR which is called whenever the timer has expired. This function
 * must then call pxMBPortCBTimerExpired( ) to notify the protocol stack that
 * the timer has expired.
 */
static void prvvTIMERExpiredISR( void )
{
    ( void )pxMBPortCBTimerExpired(  );
}

void Timer2_IRQHandler(void)
{
    // Clear interrupt flag 
    MDR_TIMER2->STATUS &= ~TIMER_IE_CNT_ARR_EVENT_IE;
    prvvTIMERExpiredISR();
}

