/*
 * ProgramFlash.hpp
 *
 */

#ifndef PROGRAMFLASH_HPP_
#define PROGRAMFLASH_HPP_

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_flash.h"
#include "fsl_common.h"


class ProgramFlash
{
public:
	ProgramFlash(uint32_t startOffset): m_startOffset(startOffset) {}

	bool open() {
		/* Clean up Flash driver Structure*/
		memset(&m_flashDriver, 0, sizeof(flash_config_t));

		FLASH_SetProperty(&m_flashDriver, kFLASH_PropertyFlashClockFrequency, ProgramFlash::CLOCK_FREQ);

		status_t result = FLASH_Init(&m_flashDriver);
		if (kStatus_FLASH_Success != result) {
			return false;
		}

		return true;
	}

	bool erase_sector(uint32_t address) {
		status_t result = FLASH_Erase(&m_flashDriver, calcDestAddress(address), sectorSize(), kFLASH_ApiEraseKey);
		return kStatus_FLASH_Success == result;
	}

	bool write(uint32_t address, uint32_t *src, size_t slen) {
		status_t result = FLASH_Program(&m_flashDriver, calcDestAddress(address), src, slen);
		return kStatus_FLASH_Success == result;
	}

	void read(uint32_t address, uint32_t *dest, size_t dlen) {
		uint32_t destAddress = calcDestAddress(address);
		for (size_t i = 0; i < dlen; i++) {
			dest[i] = *(volatile uint32_t *)(destAddress + i * 4);
		}
	}

	uint32_t totalSize() {
		return m_flashDriver.PFlashTotalSize;
	}

	uint32_t sectorSize() {
		return m_flashDriver.PFlashSectorSize;
	}

	uint32_t startAddress() {
		return m_flashDriver.PFlashBlockBase + m_startOffset;
	}

	uint32_t startOffset() {
		return m_startOffset;
	}

private:
	static constexpr uint32_t CLOCK_FREQ = 20000000U;

	uint32_t calcDestAddress(uint32_t address) {
		return startAddress() + address;
	}

	const uint32_t m_startOffset;
	flash_config_t m_flashDriver;
	//uint32_t m_verify_buffer[4];
};


#endif /* PROGRAMFLASH_HPP_ */
