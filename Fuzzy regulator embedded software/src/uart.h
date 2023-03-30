#ifndef __UART_H
#define __UART_H

#include "stdint.h"

int initUART(uint32_t);
int sendStringUART(char*);

#endif