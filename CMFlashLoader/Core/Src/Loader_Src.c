/*
 * Loader_Src.c
 *
 *  Created on: Dec 4, 2023
 *      Author: Klaus Karkov
 */

#include <stdio.h>
#include <stdbool.h>
#include "octospi.h"
#include "usart.h"
#include "gpio.h"
#include "main.h"
#include "Debug.h"
#include "FlashDriver.h"

extern void SystemClock_Config(void);
extern void MX_GPIO_Init(void);
extern void MX_LPUART1_UART_Init(void);
extern void MX_QUADSPI_Init(void);

#define LOADER_OK	0x1
#define LOADER_FAIL	0x0


int Init(void)
{
	static bool initialized = false;

	if (initialized == false)
	{
		initialized = true;

		SystemInit();

		SCB->VTOR = 0x20000000 | 0x200;

		HAL_DeInit();
		HAL_Init();

		/* Configure the system clock */
		SystemClock_Config();

		/* Initialize all configured peripherals */
		MX_GPIO_Init();
		MX_OCTOSPI1_Init();
		MX_USART3_UART_Init();

		FlashInit();

		DEBUG_PRINT("Init first\r\n");
	}
	else
	{
		DEBUG_PRINT("Init\r\n");
	}

	__set_PRIMASK(1);	//disable interrupts

	return LOADER_OK;
}

int Write(uint32_t Address, uint32_t Size, uint8_t *Buffer)
{
	DEBUG_PRINT("Write: addr: %08x size: %d\r\n", Address, Size);

	FlashClearMemoryMappedMode();

	uint32_t flashAddr = Address - FLASH_BASE_ADDR;

	int status = (FlashPageWriteBlocking(Buffer, flashAddr, Size) == HAL_OK) ? LOADER_OK : LOADER_OK;

	DEBUG_PRINT("Write: status: %d\r\n", status);

	return (status);
}

int Read(uint32_t Address, uint32_t Size, uint8_t *Buffer)
{
	int status = (FlashSetMemoryMappedMode() == HAL_OK) ? LOADER_OK : LOADER_OK;

	uint8_t* pFlash = (uint8_t*)Address;

	DEBUG_PRINT("Read: addr: %08x size: %d status: %d\r\n", Address, Size, status);

	for (uint32_t i = 0; i < Size; i++)
	{
		Buffer[i] = pFlash[i];
	}

	FlashClearMemoryMappedMode();

	return (status);
}

int SectorErase (uint32_t EraseStartAddress ,uint32_t EraseEndAddress)
{
	DEBUG_PRINT("SectorErase: startAddr: %08x endAddr: %08x\r\n", EraseStartAddress, EraseEndAddress);

	FlashClearMemoryMappedMode();

	uint32_t flashStartAddr = EraseStartAddress - FLASH_BASE_ADDR;
	uint32_t flashEndAddr   = EraseEndAddress - FLASH_BASE_ADDR;

	int status = (FlashEraseRange(flashStartAddr, flashEndAddr) == HAL_OK) ? LOADER_OK : LOADER_OK;

	DEBUG_PRINT("SectorErase: status: %d\r\n", status);

	return (status);
}

int MassErase(void)
{
	DEBUG_PRINT("MassErase\r\n");

	FlashClearMemoryMappedMode();

	int status = (FlashEraseChip() == HAL_OK) ? LOADER_OK : LOADER_OK;

	DEBUG_PRINT("MassErase: status: %d\r\n", status);

	return (status);
}

uint32_t CheckSum(uint32_t StartAddress, uint32_t Size, uint32_t InitVal)
{
	DEBUG_PRINT("CheckSum\r\n");
	return (LOADER_OK);
}

int Verify (uint32_t MemoryAddr, uint32_t RAMBufferAddr, uint32_t Size)
{
	DEBUG_PRINT("Verify\r\n");
	return (LOADER_OK);
}
