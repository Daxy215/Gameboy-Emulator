#include "MBC0.h"

MBC0::MBC0(Cartridge cartridge, std::vector<uint8_t> rom) {
	this->rom = rom;
	
	eram.resize(static_cast<size_t>(cartridge.ramSize) * 1024);
}

uint8_t MBC0::fetch8(uint16_t address) {
	if(address <= 0x7FFF)
		return rom[address];
	else if (address >= 0xA000 && address <= 0xBFFF) {
		return eram[address - 0xA000];
	}
	
	return 0xFF;
}

void MBC0::write8(uint16_t address, uint8_t data) {
	/*if(address <= 0x7FFF)
		
		//rom[address] = data;
	else */if (address >= 0xA000 && address <= 0xBFFF) {
		eram[address - 0xA000] = data;
	}
}
