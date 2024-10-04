#include "MBC3.h"

MBC3::MBC3(Cartridge cartridge, std::vector<uint8_t> rom) {
	this->rom = rom;
	
	this->romBanks = cartridge.romBanks;
	this->ramBanks = cartridge.ramBanks;
	
	//eram.resize(static_cast<size_t>(cartridge.ramSize) * 1024);
	eram.resize(static_cast<size_t>(cartridge.ramSize) * 1024);
}

uint8_t MBC3::fetch8(uint16_t address) {
	if (address >= 0xA000 && address < 0xBFFF) {
		if(!ramEnabled)
			return 0xFF;
		
		uint8_t bank = bankingMode ? curRamBank : 0;
		
		return eram[(bank * 0x2000) | (address & 0x1FFF)];
	} else {
		size_t bank;
		
		if(address < 0x4000) {
			bank = bankingMode ? (curRomBank & 0xE0) : 0;
		} else {
			bank = curRomBank;
		}
		
		size_t addr = (bank * 0x4000) | (static_cast<size_t>(address) & 0x3FFF);
		return rom[addr];
	}
	
	return 0xFF;
}

void MBC3::write8(uint16_t address, uint8_t data) {
	if(address < 0x1FFF) {
		ramEnabled = (data & 0xF) == 0xA;
	} else if(address <= 0x3FFF) {
		uint8_t lowerBits = (data & 0x1F);
		
		if(lowerBits == 0)
			lowerBits = 1;
		
		curRomBank = ((curRomBank & 0x60) | lowerBits) % romBanks;
	} else if(address >= 0x4000 && address < 0x5FFF) {
		if(romBanks > 0x20) {
			uint16_t upperBits = (data & 0x03) % (romBanks >> 5);
			curRomBank = (curRomBank & 0x1F) | (upperBits << 5);
		}
		
		if(ramBanks > 1) {
			curRamBank = data & 0x03;
		}
	} else if(address >= 0x6000 && address < 0x7FFF) {
		bankingMode = data & 0x01;
	} else if(address >= 0xA000 && address <= 0xBFFF) {
		if(!ramEnabled) {
			return;
		}
		
		uint8_t bank = bankingMode ? (curRamBank) : 0;
		
		eram[(address & 0x1FFF) + (bank * 0x2000)] = data;
	}
}
