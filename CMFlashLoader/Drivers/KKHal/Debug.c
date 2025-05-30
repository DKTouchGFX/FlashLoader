/*
 * Debug.c
 *
 *  Created on: 3. maj 2019
 *      Author: Klaus
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "Debug.h"

extern UART_HandleTypeDef huart3;

void DebugPrint(const char* format, ...)
{
    va_list args;
    char tempBuffer[400];

    va_start(args, format);

    vsprintf((char*)tempBuffer, format, args);

    va_end(args);

    tempBuffer[sizeof(tempBuffer) - 1] = 0; // Make sure strlen does not overflow

    HAL_UART_Transmit(&huart3, (uint8_t *)tempBuffer, strlen(tempBuffer), 200);
}
