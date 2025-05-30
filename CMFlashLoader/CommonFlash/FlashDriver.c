#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "Debug.h"
#include "FlashDriver.h"

#define QSPI_HANDLE hospi1

extern OSPI_HandleTypeDef           QSPI_HANDLE;

#define FLASH_ADDR_SIZE             26
#define FLASH_WRITE_PAGE_SIZE       256
#define FLASH_BLOCK_SIZE            (64 * 1024)

#define FLASH_BLOCK_CNT             1024

/* Flash commands */
#define QSPI_IO_READ_CMD            0xeb
#define QSPI_PAGE_PROG_CMD          0x02
#define QSPI_READ_STATUS_REG_CMD    0x05
#define QSPI_WRITE_STATUS_REG_CMD   0x01
#define QSPI_READ_BANK_ADDRESS_REG_CMD   0x16
#define QSPI_WRITE_BANK_ADDRESS_REG_CMD   0x18
#define QSPI_SECTOR_ERASE_CMD       0xd7
#define QSPI_BLOCK32_ERASE_CMD      0x52
#define QSPI_BLOCK64_ERASE_CMD      0xd8
#define QSPI_CHIP_ERASE_CMD         0xc7
#define QSPI_WRITE_ENABLE_CMD       0x06
#define QSPI_ENTER_QSPI_MODE_CMD    0x35

/* Dummy clocks cycles */
#define DUMMY_CLOCK_CYCLES_READ        6
#define DUMMY_CLOCK_CYCLES_WRITE       0

/* Auto-polling values */
#define WRITE_ENABLE_MATCH_VALUE    0x02
#define WRITE_ENABLE_MASK_VALUE     0x02

// WIP Write in progress bit
#define MEMORY_READY_MATCH_VALUE    0x00
#define MEMORY_READY_MASK_VALUE     0x01

// WIP Write in progress bit
#define QUAD_ENABLE_MASK_VALUE      0x40

/* End address of the OSPI memory */
#define FLASH_END_ADDR               (1 << FLASH_ADDR_SIZE)

#define SET_SPI_CMD(_sCmd, _inst, _addr,  _addrMode, _dataMode, _nbData, _dummyCycles) \
_sCmd.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;                               \
_sCmd.FlashId            = HAL_OSPI_FLASH_ID_1;                                      \
_sCmd.Instruction        = _inst;                                                    \
_sCmd.InstructionMode    = HAL_OSPI_INSTRUCTION_4_LINES;                             \
_sCmd.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;                              \
_sCmd.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;                         \
_sCmd.Address            = _addr;                                                    \
_sCmd.AddressMode        = _addrMode;                                                \
_sCmd.AddressSize        = HAL_OSPI_ADDRESS_32_BITS;                                 \
_sCmd.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;                             \
_sCmd.AlternateBytes     = 0;                                                        \
_sCmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;                            \
_sCmd.AlternateBytesSize = 0;                                                        \
_sCmd.AlternateBytesDtrMode = 0;                                                     \
_sCmd.DataMode           = _dataMode;                                                \
_sCmd.NbData             = _nbData;                                                  \
_sCmd.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;                                \
_sCmd.DummyCycles        = _dummyCycles;                                             \
_sCmd.DQSMode            = HAL_OSPI_DQS_DISABLE;                                     \
_sCmd.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;                             \

static uint8_t FlashWriteEnable(OSPI_HandleTypeDef *hospi);
uint8_t FlashSetMemoryMappedMode(void);
void FlashClearMemoryMappedMode(void);
bool FlashIsInMemoryMappedMode(void);

uint8_t FlashReadStatusReg(void);
void FlashWriteStatusReg(uint8_t val);
uint8_t FlashWaitForStatusRegisterFlag(uint8_t matchFlag, uint8_t maskFlag);

void FlashUpdateStatusReg(void);
uint8_t FlashReadBankAddressReg(void);
uint8_t FlashWriteBankAddressReg(void);
void FlaseEnterQSpiMode(void);
bool FlashAddrOk(uint32_t endAddr);

bool gQSpiEnabled;

void FlashInit(void)
{
	gQSpiEnabled = false;

	FlaseEnterQSpiMode();
	FlashUpdateStatusReg();
	FlashWriteBankAddressReg();
	FlashSetMemoryMappedMode();
}

uint8_t FlashWriteBankAddressReg(void)
{
    OSPI_RegularCmdTypeDef sCommand;
	uint8_t status = HAL_OK;
	uint8_t val = 0x80;

    FlashWriteEnable(&QSPI_HANDLE);

    SET_SPI_CMD(sCommand, QSPI_WRITE_BANK_ADDRESS_REG_CMD, 0, HAL_OSPI_ADDRESS_NONE, HAL_OSPI_DATA_4_LINES, 1, 0);

	status = HAL_OSPI_Command(&QSPI_HANDLE, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

	if (status != HAL_OK)
	{
		DebugPrint("FlashWriteBankAddressReg failed1: %d errorCode: %d\r", status, QSPI_HANDLE.ErrorCode);
	}

	if (status == HAL_OK)
	{
		status = HAL_OSPI_Transmit(&QSPI_HANDLE, &val, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
	}

	if (status != HAL_OK)
	{
		DebugPrint("FlashWriteBankAddressReg failed2: %d\r", status);
	}

	if (status == HAL_OK)
	{
		status = FlashWaitForStatusRegisterFlag(MEMORY_READY_MATCH_VALUE, MEMORY_READY_MASK_VALUE);
	}

    return (status);
}

uint8_t FlashReadBankAddressReg(void)
{
	OSPI_RegularCmdTypeDef sCommand;
	uint8_t val = -1;
	uint8_t status = HAL_OK;

	SET_SPI_CMD(sCommand, QSPI_READ_BANK_ADDRESS_REG_CMD, 0, HAL_OSPI_ADDRESS_NONE, HAL_OSPI_DATA_4_LINES, 1, 0);

	status = HAL_OSPI_Command(&QSPI_HANDLE, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

	if (status != HAL_OK)
	{
		DebugPrint("FlashReadStatusReg failed1: %d\r", status);
	}

	if (status == HAL_OK)
	{
		status = HAL_OSPI_Receive(&QSPI_HANDLE, &val, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
	}

	if (status != HAL_OK)
	{
		DebugPrint("FlashReadStatusReg failed2: %d\r", status);
		val = -1;
	}

    return (val);
}

void FlashUpdateStatusReg(void)
{
	uint8_t statusReg = FlashReadStatusReg();

	if ((statusReg & QUAD_ENABLE_MASK_VALUE) == 0)
	{
		FlashWriteEnable(&QSPI_HANDLE);

		statusReg |= QUAD_ENABLE_MASK_VALUE;

		FlashWriteStatusReg(statusReg);

		statusReg = FlashReadStatusReg();

		DebugPrint("StatusReg2: %02x\r", statusReg);
	}
}

uint8_t FlashErase(uint32_t addr, uint32_t eraseCmd)
{
    OSPI_RegularCmdTypeDef sCommand;
	uint8_t status = HAL_OK;

    SET_SPI_CMD(sCommand, eraseCmd, addr, HAL_OSPI_ADDRESS_4_LINES, HAL_OSPI_DATA_NONE, 0, 0);

    FlashWriteEnable(&QSPI_HANDLE);

	status = HAL_OSPI_Command(&QSPI_HANDLE, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

	if (status != HAL_OK)
	{
		DebugPrint("FlashErase failed1: %d - cmd: %02x\r", status, eraseCmd);
	}

	status = FlashWaitForStatusRegisterFlag(MEMORY_READY_MATCH_VALUE, MEMORY_READY_MASK_VALUE);

	if (status != HAL_OK)
	{
		DebugPrint("FlashErase failed2: %d - cmd: %02x\r", status, eraseCmd);
	}

    return (status);
}


uint8_t FlashErase4KSector(uint32_t addr)
{
	return (FlashErase(addr, QSPI_SECTOR_ERASE_CMD));
}

uint8_t FlashErase32KBlock(uint32_t addr)
{
	return (FlashErase(addr, QSPI_BLOCK32_ERASE_CMD));
}

uint8_t FlashErase64KBlock(uint32_t addr)
{
	return (FlashErase(addr, QSPI_BLOCK64_ERASE_CMD));
}

uint8_t FlashEraseChip(void)
{
    OSPI_RegularCmdTypeDef sCommand;
	uint8_t status = HAL_OK;

	DebugPrint("FlashEraseChip start\r");

    SET_SPI_CMD(sCommand, QSPI_CHIP_ERASE_CMD, 0, HAL_OSPI_ADDRESS_NONE, HAL_OSPI_DATA_NONE, 0, 0);

    FlashWriteEnable(&QSPI_HANDLE);

	status = HAL_OSPI_Command(&QSPI_HANDLE, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

	if (status != HAL_OK)
	{
		DebugPrint("FlashEraseChip failed1: %d\r", status);
	}

	status = FlashWaitForStatusRegisterFlag(MEMORY_READY_MATCH_VALUE, MEMORY_READY_MASK_VALUE);

	if (status != HAL_OK)
	{
		DebugPrint("FlashEraseChip failed3: %d\r", status);
	}

	DebugPrint("FlashEraseChip end\r");

    return (status);
}

#define FLASH_32K_BLOCK_SIZE (32 * 1024)
#define FLASH_64K_BLOCK_SIZE (64 * 1024)

uint8_t FlashEraseRange(uint32_t startAddr, uint32_t endAddr)
{
	uint8_t status = HAL_OK;
	uint32_t addr  = startAddr & -FLASH_SECTOR_SIZE; // Allign with 4K address
	uint32_t endAddrAlligned  = endAddr;

	if ((endAddrAlligned & (FLASH_SECTOR_SIZE - 1)) != 0)
	{
		endAddrAlligned = (endAddrAlligned & -FLASH_SECTOR_SIZE) + FLASH_SECTOR_SIZE;
	}

	if (addr == endAddrAlligned)
	{
		endAddrAlligned += FLASH_SECTOR_SIZE;
	}

	DebugPrint("FlashEraseRange - start: addr: %08x endAddr: %08x startAddr: %08x allEndAddr: %08x\r", startAddr, endAddr, addr, endAddrAlligned);

	if (FlashAddrOk(endAddrAlligned) == true)
	{
		while ((addr < endAddrAlligned) && (status == HAL_OK))
		{
			uint32_t remaining = endAddrAlligned - addr;

			if ((remaining >= FLASH_64K_BLOCK_SIZE) && ((addr & (FLASH_64K_BLOCK_SIZE - 1)) == 0))
			{
				DebugPrint("FlashEraseRange - Erase 64K: addr: %08x remaining: %d\r", addr, remaining);
				FlashErase64KBlock(addr);
				addr += FLASH_64K_BLOCK_SIZE;
			}
			else if ((remaining >= FLASH_32K_BLOCK_SIZE) && ((addr & (FLASH_32K_BLOCK_SIZE - 1)) == 0))
			{
				DebugPrint("FlashEraseRange - Erase 32K: addr: %08x remaining: %d\r", addr, remaining);
				FlashErase32KBlock(addr);
				addr += FLASH_32K_BLOCK_SIZE;
			}
			else if ((remaining >= FLASH_SECTOR_SIZE) && ((addr & (FLASH_SECTOR_SIZE - 1)) == 0))
			{
				DebugPrint("FlashEraseRange - Erase  4K: addr: %08x remaining: %d\r", addr, remaining);
				FlashErase4KSector(addr);
				addr += FLASH_SECTOR_SIZE;
			}
			else
			{
				DebugPrint("FlashEraseRange - Panic: addr: %08x remaining: %d\r", addr, remaining);
				status = HAL_ERROR;
			}
		}
    }
    else
    {
    	status = HAL_ERROR;
    }

	return (status);
}


uint8_t FlashPageWriteBlocking(uint8_t* pData, uint32_t addr, uint32_t size)
{
	OSPI_RegularCmdTypeDef sCommand;
	uint8_t status = HAL_OK;;

	FlashWriteEnable(&QSPI_HANDLE);

	SET_SPI_CMD(sCommand, QSPI_PAGE_PROG_CMD, addr, HAL_OSPI_ADDRESS_4_LINES, HAL_OSPI_DATA_4_LINES, size, DUMMY_CLOCK_CYCLES_WRITE);

	status = HAL_OSPI_Command(&QSPI_HANDLE, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

	if (status != HAL_OK)
	{
		DebugPrint("FlashPageWriteBlocking failed1: %d errorCode: %d\r", status, QSPI_HANDLE.ErrorCode);
	}

	if (status == HAL_OK)
	{
		status = HAL_OSPI_Transmit(&QSPI_HANDLE, pData, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

		if (status != HAL_OK)
		{
			DebugPrint("FlashPageWriteBlocking failed2: %d errorCode: %d addr: %08x\r", status, QSPI_HANDLE.ErrorCode, addr);
		}

		if (status == HAL_OK)
		{
			status = FlashWaitForStatusRegisterFlag(MEMORY_READY_MATCH_VALUE, MEMORY_READY_MASK_VALUE);
		}
	}

	return (status);
}

bool FlashIsInMemoryMappedMode(void)
{
	return (QSPI_HANDLE.State == HAL_OSPI_STATE_BUSY_MEM_MAPPED);
}

uint8_t FlashSetMemoryMappedMode(void)
{
	OSPI_MemoryMappedTypeDef sMemMappedCfg;
    OSPI_RegularCmdTypeDef   sCommand;
	uint8_t status = HAL_OK;

	if (FlashIsInMemoryMappedMode() == false)
	{
		SET_SPI_CMD(sCommand, QSPI_PAGE_PROG_CMD, 0, HAL_OSPI_ADDRESS_4_LINES, HAL_OSPI_DATA_4_LINES, 0, DUMMY_CLOCK_CYCLES_WRITE);
		sCommand.OperationType = HAL_OSPI_OPTYPE_WRITE_CFG;

		status = HAL_OSPI_Command(&QSPI_HANDLE, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

		if (status != HAL_OK)
		{
			DebugPrint("FlashSetMemoryMappedMode failed0: %d ErrorCode: %d state: %d\r", status, QSPI_HANDLE.ErrorCode, QSPI_HANDLE.State);
		}

		SET_SPI_CMD(sCommand, QSPI_IO_READ_CMD, 0,  HAL_OSPI_ADDRESS_4_LINES, HAL_OSPI_DATA_4_LINES, 0, DUMMY_CLOCK_CYCLES_READ);
		sCommand.OperationType = HAL_OSPI_OPTYPE_READ_CFG;

		status = HAL_OSPI_Command(&QSPI_HANDLE, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

		if (status != HAL_OK)
		{
			DebugPrint("FlashSetMemoryMappedMode failed1: %d ErrorCode: %d state: %d\r", status, QSPI_HANDLE.ErrorCode, QSPI_HANDLE.State);
		}

		if (status == HAL_OK)
		{
			sMemMappedCfg.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_ENABLE;
			sMemMappedCfg.TimeOutPeriod     = 0x40;

			uint8_t status = HAL_OSPI_MemoryMapped(&QSPI_HANDLE, &sMemMappedCfg);

			if (status != HAL_OK)
			{
				DebugPrint("FlashSetMemoryMappedMode failed2: %d ErrorCode: %d state: %d\r", status, QSPI_HANDLE.ErrorCode, QSPI_HANDLE.State);
			}
		}

		if (status == HAL_OK)
		{
			// DebugPrint("FlashSetMemoryMappedMode success\r");
		}
	}
	return ( status );
}

void FlashClearMemoryMappedMode(void)
{
	if (FlashIsInMemoryMappedMode() == true)
	{
		uint8_t status = HAL_OSPI_Abort(&QSPI_HANDLE);

		if (status != HAL_OK)
		{
			DebugPrint("FlashClearMemoryMappedMode failed: %d ErrorCode: %d state: %d\r", status, QSPI_HANDLE.ErrorCode, QSPI_HANDLE.State);
		}
	}
}

uint8_t FlashReadStatusReg(void)
{
	OSPI_RegularCmdTypeDef sCommand;
	uint8_t val = -1;
	uint8_t status = HAL_OK;

	SET_SPI_CMD(sCommand, QSPI_READ_STATUS_REG_CMD, 0, HAL_OSPI_ADDRESS_NONE, HAL_OSPI_DATA_4_LINES, 1, 0);

	if (gQSpiEnabled == false)
	{
		sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
		sCommand.DataMode = HAL_OSPI_DATA_1_LINE;
	}

	status = HAL_OSPI_Command(&QSPI_HANDLE, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

	if (status != HAL_OK)
	{
		DebugPrint("FlashReadStatusReg failed1: %d\r", status);
	}

	if (status == HAL_OK)
	{
		status = HAL_OSPI_Receive(&QSPI_HANDLE, &val, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
	}

	if (status != HAL_OK)
	{
		DebugPrint("FlashReadStatusReg failed2: %d\r", status);
		val = -1;
	}

    return (val);
}

void FlaseEnterQSpiMode(void)
{
	OSPI_RegularCmdTypeDef sCommand;
	uint8_t status = HAL_OK;

	SET_SPI_CMD(sCommand, QSPI_ENTER_QSPI_MODE_CMD, 0, HAL_OSPI_ADDRESS_NONE, HAL_OSPI_DATA_NONE, 0, 0);

	if (gQSpiEnabled == false)
	{
		sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
	}

	status = HAL_OSPI_Command(&QSPI_HANDLE, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

	if (status != HAL_OK)
	{
		DebugPrint("FlaseEnterQSpiMode failed1: %d errorCode: %d\r", status, QSPI_HANDLE.ErrorCode);
	}

	gQSpiEnabled = true;

	if (status == HAL_OK)
	{
		status = FlashWaitForStatusRegisterFlag(MEMORY_READY_MATCH_VALUE, MEMORY_READY_MASK_VALUE);
	}
}

void FlashWriteStatusReg(uint8_t val)
{
	OSPI_RegularCmdTypeDef sCommand;
	uint8_t status = HAL_OK;

	SET_SPI_CMD(sCommand, QSPI_WRITE_STATUS_REG_CMD, 0, HAL_OSPI_ADDRESS_NONE, HAL_OSPI_DATA_4_LINES, 1, 0);

	status = HAL_OSPI_Command(&QSPI_HANDLE, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

	if (status != HAL_OK)
	{
		DebugPrint("FlashWriteStatusReg failed1: %d errorCode: %d\r", status, QSPI_HANDLE.ErrorCode);
	}

	if (status == HAL_OK)
	{
		status = HAL_OSPI_Transmit(&QSPI_HANDLE, &val, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
	}

	if (status != HAL_OK)
	{
		DebugPrint("FlashWriteStatusReg failed2: %d\r", status);
	}

	if (status == HAL_OK)
	{
		status = FlashWaitForStatusRegisterFlag(MEMORY_READY_MATCH_VALUE, MEMORY_READY_MASK_VALUE);
	}
}

uint8_t FlashWaitForStatusRegisterFlag(uint8_t matchFlag, uint8_t maskFlag)
{
	uint8_t statusReg;

	do
	{
		statusReg = FlashReadStatusReg();
	}
	while ((statusReg & maskFlag) != matchFlag);

	return (HAL_OK);
}

bool FlashAddrOk(uint32_t endAddr)
{
	bool addrOk = true;

	if (endAddr > FLASH_TOTAL_SIZE)
	{
		addrOk = false;
		DebugPrint("Flash address outside range: %08x\r", endAddr);
	}

	return (addrOk);
}

uint8_t FlashWrite(uint8_t* pData, uint32_t addr, uint32_t size)
{
  uint8_t  status = HAL_ERROR;
  uint32_t currAddr = addr;
  uint32_t remainingSize = size;

  // DebugPrint("FlashWrite: addr: %4d size: %4d\r", addr, size);

  if (FlashAddrOk(addr + size) == true)
  {
	  /* Calculation of the size between the write address and the end of the page */
	  uint32_t pageOffset = addr % FLASH_WRITE_PAGE_SIZE;
	  uint32_t currSize   = FLASH_WRITE_PAGE_SIZE - pageOffset;

	  /* Perform the write page by page */
	  do
	  {
		if (currSize > remainingSize)
		{
		  currSize = remainingSize;
		}

		status = FlashPageWriteBlocking(pData, currAddr, currSize);

		currAddr += currSize;
		pData += currSize;
		remainingSize -= currSize;
		currSize = FLASH_WRITE_PAGE_SIZE;
	  }
	  while ((remainingSize > 0) && (status == HAL_OK));
  }

  return (status);
}

static uint8_t FlashWriteEnable(OSPI_HandleTypeDef *hospi)
{
	OSPI_RegularCmdTypeDef  sCommand;
	uint8_t status = HAL_OK;

	SET_SPI_CMD(sCommand, QSPI_WRITE_ENABLE_CMD, 0, HAL_OSPI_ADDRESS_NONE, HAL_OSPI_DATA_NONE, 0, 0);

	status = HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

	if (status != HAL_OK)
	{
		DebugPrint("FlashWriteEnable failed1: %d\r", status);
	}

	if (status == HAL_OK)
	{
		status = FlashWaitForStatusRegisterFlag(WRITE_ENABLE_MATCH_VALUE, WRITE_ENABLE_MASK_VALUE);
	}

	return (status);
}

#define TEST_BUF_SIZE 1024

uint32_t txBuf[TEST_BUF_SIZE];
// #define TEST_BUF_CNT  (FLASH_TOTAL_SIZE / sizeof(txBuf))
// Klaus test #define TEST_BUF_CNT  4

// The 64 MByte is filled by 16 * 1024 * 1024 32bit words
#define TEST_BUF_CNT  (16 * 1024)

#define TEST_START_ADDR  0x4000
#define TEST_END_ADDR   0x16000

void FlashTest(void)
{
	uint8_t status = HAL_OK;
	uint32_t val = 0;
	uint32_t addr = TEST_START_ADDR;

	FlashInit();

	DebugPrint("\r");

	FlashClearMemoryMappedMode();
	status = FlashEraseRange(TEST_START_ADDR, TEST_END_ADDR);

	// status = FlashEraseChip();

	if (status == HAL_OK)
	{
		DebugPrint("Erase success\r");
	}
	else
	{
		DebugPrint("Erase failed %d\r", status);
	}

	if (status == HAL_OK)
	{
		while ((addr < TEST_END_ADDR) && (status == HAL_OK))
		{
			for (uint32_t i = 0; i < TEST_BUF_SIZE; i++)
			{
				txBuf[i] = val;
				val++;
			}

			uint32_t remaining = TEST_END_ADDR - addr;
			uint32_t bytesToProgram = sizeof(txBuf);

			if (remaining < sizeof(txBuf))
			{
				bytesToProgram = remaining;
			}

			status = FlashWrite((uint8_t*)txBuf, addr, bytesToProgram);

			addr += bytesToProgram;

			DebugPrint("w");
		}

		DebugPrint("\r");

		if (status == HAL_OK)
		{
			DebugPrint("Write success\r");
		}
		else
		{
			DebugPrint("Write failed %d\r", status);
		}

		if (status == HAL_OK)
		{
			if (FlashSetMemoryMappedMode() == HAL_OK)
			{
				val = 0;
				addr = TEST_START_ADDR;

				uint32_t* pFlashBaseAddr = (uint32_t*)(FLASH_BASE_ADDR + TEST_START_ADDR);

				while (addr < TEST_END_ADDR)
				{
					if (pFlashBaseAddr[val] != val)
					{
						DebugPrint("f");
					}
					addr += 4;
					val++;
				}

				DebugPrint("Read end\r");
			}
		}
	}
}
