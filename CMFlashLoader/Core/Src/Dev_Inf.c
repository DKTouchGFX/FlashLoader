/*
 * Dev_Inf.c
 *
 */
#include "Dev_Inf.h"


#define MEMORY_FLASH_SIZE (64 * 1024 * 1024)
#define MEMORY_PAGE_SIZE  256

/* This structure contains information used by ST-LINK Utility to program and erase the device */
#if defined (__ICCARM__)
__root struct StorageInfo const StorageInfo  =  {
#else
struct StorageInfo __attribute__((section(".Dev_info"))) /*const*/ StorageInfo  = {
#endif
		"KK Data IS25LP512M",	             // Device Name + version number
		SPI_FLASH,                  		 // Device Type
		0x90000000,               			 // Device Start Address
		MEMORY_FLASH_SIZE,                 	 // Device Size in Bytes
		MEMORY_PAGE_SIZE,                    // Programming Page Size
		0xFF,                                // Initial Content of Erased Memory

		// Specify Size and Address of Sectors
		{
			{ 0x00004000, 0x00001000 }, // Sector Num : (16 * 1024) ,Sector Size: 4 KBytes
			{ 0x00000000, 0x00000000 }
		}
};
