#pragma once

#include "../../MBC.h"

class MBC1 : public MBC {
public:
	MBC1() = default;
	MBC1(Cartridge cartridge, std::vector<uint8_t> rom);
	
	uint8_t fetch8(uint16_t address) override;
	void write8(uint16_t address, uint8_t data) override;
	
private:
	/**
	 * 0 - ROM
	 * 1 - RAM
	 */
	bool bankingMode = false;
	bool ramEnabled = false;
	
	uint8_t curRomBank = 0;
	uint8_t curRamBank = 0;
};
