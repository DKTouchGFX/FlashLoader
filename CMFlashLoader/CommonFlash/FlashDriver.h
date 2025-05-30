#ifndef FLASHDRIVER_H_
#define FLASHDRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"


#define FLASH_SECTOR_SIZE  (4 * 1024)

#define FLASH_BASE_ADDR  OCTOSPI1_BASE
#define FLASH_TOTAL_SIZE (64 * 1024 * 1024)

void FlashInit(void);
uint8_t FlashSetMemoryMappedMode(void);
void FlashClearMemoryMappedMode(void);

uint8_t FlashErase4KSector(uint32_t addr);
uint8_t FlashErase32KBlock(uint32_t addr);
uint8_t FlashErase64KBlock(uint32_t addr);
uint8_t FlashEraseChip(void);

uint8_t FlashEraseRange(uint32_t startAddr, uint32_t endAddr);

uint8_t FlashWrite(uint8_t* pData, uint32_t addr, uint32_t size);
uint8_t FlashRead(uint8_t* pData, uint32_t addr, uint32_t size);
uint8_t FlashPageWriteBlocking(uint8_t* pData, uint32_t addr, uint32_t size);

void FlashTest(void);

#ifdef __cplusplus
}
#endif


#endif /* FLASHDRIVER_H_ */
