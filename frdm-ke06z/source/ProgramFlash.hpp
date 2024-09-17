/*
 *  Copyright (C) 2020-2024 Embedded AMS B.V. - All Rights Reserved
 *
 *  This file is part of Embedded Proto.
 *
 *  Embedded Proto is open source software: you can redistribute it and/or 
 *  modify it under the terms of the GNU General Public License as published 
 *  by the Free Software Foundation, version 3 of the license.
 *
 *  Embedded Proto  is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Embedded Proto. If not, see <https://www.gnu.org/licenses/>.
 *
 *  For commercial and closed source application please visit:
 *  <https://EmbeddedProto.com/license/>.
 *
 *  Embedded AMS B.V.
 *  Info:
 *    info at EmbeddedProto dot com
 *
 *  Postal address:
 *    Atoomweg 2
 *    1627 LE, Hoorn
 *    the Netherlands
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
};


#endif /* PROGRAMFLASH_HPP_ */
