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

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- Platform specific includes -----------------------*/
#include "MDR32FxQI_uart.h"
#include "MDR32FxQI_rst_clk.h"
#include "MDR32FxQI_port.h"

/* ----------------------- static functions ---------------------------------*/
static void prvvUARTTxReadyISR( void );
static void prvvUARTRxISR( void );
static void __PORT_Init(MDR_PORT_TypeDef* PORTx, const PORT_InitTypeDef* PORT_InitStruct);

/* ----------------------- Start implementation -----------------------------*/
void
vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
    /* If xRXEnable enable serial receive interrupts. If xTxENable enable
     * transmitter empty interrupts.
     */
    if(xRxEnable == TRUE)
    {
        UART_ITConfig(MDR_UART2, UART_IT_RX, ENABLE);//включим прерывание если входной буфер не пуст
    }
    else
    {
        UART_ITConfig(MDR_UART2, UART_IT_RX, DISABLE);//выключим прерывание если входной буфер не пуст
        UART_ClearITPendingBit(MDR_UART2, UART_IT_RX);
    } 
  
    if(xTxEnable == TRUE)
    {
        UART_ITConfig(MDR_UART2, UART_IT_TX, ENABLE);//включим прерывание если выходной буфер пуст
    }
    else
    {
        UART_ITConfig(MDR_UART2, UART_IT_TX, DISABLE);//выключим прерывание если выходной буфер пуст
        UART_ClearITPendingBit(MDR_UART2, UART_IT_TX);
    }
}

BOOL
xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{
    UART_InitTypeDef UART_InitStructure;
    PORT_InitTypeDef port;

    // Enable UART2 clocking
	RST_CLK_PCLKcmd(RST_CLK_PCLK_UART2, ENABLE);
    // Enable PORTD clocking
    RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTF, ENABLE);

    // Fill PortInit structure
	port.PORT_PULL_UP = PORT_PULL_UP_OFF;
	port.PORT_PULL_DOWN = PORT_PULL_DOWN_OFF;
	port.PORT_PD_SHM = PORT_PD_SHM_OFF;
	port.PORT_PD = PORT_PD_DRIVER;
	port.PORT_GFEN = PORT_GFEN_OFF;
	port.PORT_FUNC = PORT_FUNC_OVERRID;
	port.PORT_SPEED = PORT_SPEED_SLOW;
	port.PORT_MODE = PORT_MODE_DIGITAL;

	// Configure PORTD pins 1 (UART2_TX) as output 
	port.PORT_OE = PORT_OE_OUT;
	port.PORT_Pin = PORT_Pin_1;
	PORT_Init(MDR_PORTF, &port);

	// Configure PORTD pins 0 (UART2_RX) as input 
	port.PORT_OE = PORT_OE_IN;
	port.PORT_Pin = PORT_Pin_0;
	PORT_Init(MDR_PORTF, &port);

    UART_BRGInit(MDR_UART2, UART_HCLKdiv8); 

    // Initialize UART_InitStructure
	UART_InitStructure.UART_BaudRate = ulBaudRate;
	UART_InitStructure.UART_WordLength = UART_WordLength8b;
	UART_InitStructure.UART_StopBits = UART_StopBits1;
	UART_InitStructure.UART_Parity = UART_Parity_No;
	UART_InitStructure.UART_FIFOMode = UART_FIFO_OFF;
	UART_InitStructure.UART_HardwareFlowControl = UART_HardwareFlowControl_RXE
			| UART_HardwareFlowControl_TXE;

	// Configure UART2 parameters
	UART_Init(MDR_UART2, &UART_InitStructure);
    // Enable interrupts for TX and RX
    UART_ITConfig(MDR_UART2, UART_IT_TX, ENABLE);
    UART_ITConfig(MDR_UART2, UART_IT_RX, ENABLE);
    // Globally init interrupt vector
    NVIC_EnableIRQ(UART2_IRQn);
	// Enables UART2 peripheral
	UART_Cmd(MDR_UART2, ENABLE);

    return TRUE;
}

BOOL
xMBPortSerialPutByte( CHAR ucByte )
{
    /* Put a byte in the UARTs transmit buffer. This function is called
     * by the protocol stack if pxMBFrameCBTransmitterEmpty( ) has been
     * called. */

    while (UART_GetFlagStatus(MDR_UART2, UART_FLAG_TXFE ) != SET);
	UART_SendData(MDR_UART2, ucByte);

    return TRUE;
}

BOOL
xMBPortSerialGetByte( CHAR * pucByte )
{
    /* Return the byte in the UARTs receive buffer. This function is called
     * by the protocol stack after pxMBFrameCBByteReceived( ) has been called.
     */
	*pucByte = (uint8_t) UART_ReceiveData(MDR_UART2);

    return TRUE;
}

void UART2_IRQHandler(void)
{
  /* Событие при передаче байта ------------------------------------------------*/
  if((UART_GetITStatus(MDR_UART2,UART_IT_TX)) !=RESET)
  {
    prvvUARTTxReadyISR(  );
  } 

  /* Событие при приеме байта ---------------------------------------------------*/
  if((UART_GetITStatus(MDR_UART2,UART_IT_RX)) != RESET)
  { 
      prvvUARTRxISR(  ); 
  }
}


/* Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call 
 * xMBPortSerialPutByte( ) to send the character.
 */
static void prvvUARTTxReadyISR( void )
{
    pxMBFrameCBTransmitterEmpty(  );
}

/* Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
static void prvvUARTRxISR( void )
{
    pxMBFrameCBByteReceived(  );
}

static void __PORT_Init(MDR_PORT_TypeDef* PORTx, const PORT_InitTypeDef* PORT_InitStruct)
{
  uint32_t tmpreg_OE;
  uint32_t tmpreg_FUNC;
  uint32_t tmpreg_ANALOG;
  uint32_t tmpreg_PULL;
  uint32_t tmpreg_PD;
  uint32_t tmpreg_PWR;
  uint32_t tmpreg_GFEN;
  uint32_t portpin, pos, mask_s, mask_l, mask_d;

  /* Check the parameters */
  assert_param(IS_PORT_ALL_PERIPH(PORTx));
  assert_param(IS_PORT_PIN(PORT_InitStruct->PORT_Pin));
  assert_param(IS_PORT_OE(PORT_InitStruct->PORT_OE));
  assert_param(IS_PORT_PULL_UP(PORT_InitStruct->PORT_PULL_UP));
  assert_param(IS_PORT_PULL_DOWN(PORT_InitStruct->PORT_PULL_DOWN));
  assert_param(IS_PORT_PD_SHM(PORT_InitStruct->PORT_PD_SHM));
  assert_param(IS_PORT_PD(PORT_InitStruct->PORT_PD));
  assert_param(IS_PORT_GFEN(PORT_InitStruct->PORT_GFEN));
  assert_param(IS_PORT_FUNC(PORT_InitStruct->PORT_FUNC));
  assert_param(IS_PORT_SPEED(PORT_InitStruct->PORT_SPEED));
  assert_param(IS_PORT_MODE(PORT_InitStruct->PORT_MODE));

  /* Get current PORT register values */
  tmpreg_OE     = PORTx->OE;
  tmpreg_FUNC   = PORTx->FUNC;
  tmpreg_ANALOG = PORTx->ANALOG;
  tmpreg_PULL   = PORTx->PULL;
  tmpreg_PD     = PORTx->PD;
  tmpreg_PWR    = PORTx->PWR;
  tmpreg_GFEN   = PORTx->GFEN;

  /* Form new values */
  pos = 0;
  mask_s = 0x0001;
  mask_l = 0x00000003;
  mask_d = 0x00010001;
  for (portpin = PORT_InitStruct->PORT_Pin; portpin; portpin >>= 1)
  {
    if (portpin & 0x1)
    {
      tmpreg_OE     = (tmpreg_OE     & ~mask_s) | (PORT_InitStruct->PORT_OE        <<  pos);
      tmpreg_FUNC   = (tmpreg_FUNC   & ~mask_l) | (PORT_InitStruct->PORT_FUNC      << (pos*2));
      tmpreg_ANALOG = (tmpreg_ANALOG & ~mask_s) | (PORT_InitStruct->PORT_MODE      <<  pos);
      tmpreg_PULL   = (tmpreg_PULL   & ~mask_d) | (PORT_InitStruct->PORT_PULL_UP   << (pos + 16))
                                                | (PORT_InitStruct->PORT_PULL_DOWN <<  pos);
      tmpreg_PD     = (tmpreg_PD     & ~mask_d) | (PORT_InitStruct->PORT_PD_SHM    << (pos + 16))
                                                | (PORT_InitStruct->PORT_PD        <<  pos);
      tmpreg_PWR    = (tmpreg_PWR    & ~mask_l) | (PORT_InitStruct->PORT_SPEED     << (pos*2));
      tmpreg_GFEN   = (tmpreg_GFEN   & ~mask_s) | (PORT_InitStruct->PORT_GFEN      <<  pos);
    }
    mask_s <<= 1;
    mask_l <<= 2;
    mask_d <<= 1;
    pos++;
  }
  
  PORTx->OE     = tmpreg_OE;
  PORTx->FUNC   = tmpreg_FUNC;
  PORTx->ANALOG = tmpreg_ANALOG;
  PORTx->PULL   = tmpreg_PULL;
  PORTx->PD     = tmpreg_PD;
  PORTx->PWR    = tmpreg_PWR;
  PORTx->GFEN   = tmpreg_GFEN;
}