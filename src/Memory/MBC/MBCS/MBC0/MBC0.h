#pragma once

#include "../../MBC.h"

class MBC0 : public MBC {
public:
	MBC0() = default;
	MBC0(Cartridge cartridge, std::vector<uint8_t> rom);
	
	uint8_t fetch8(uint16_t address) override;
	void write8(uint16_t address, uint8_t data) override;
};
