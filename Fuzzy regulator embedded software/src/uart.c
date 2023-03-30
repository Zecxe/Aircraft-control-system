#include "stdint.h"
#include "MDR32FxQI_uart.h"
#include "MDR32FxQI_rst_clk.h"
#include "MDR32FxQI_port.h"

int sendStringUART(char *str)
{
    while(*str) {
		while (UART_GetFlagStatus(MDR_UART2, UART_FLAG_TXFE ) != SET);
		UART_SendData(MDR_UART2, *str);
		str++;
    }
    
    return 0;
}