#include "MBC1.h"

#include <fstream>
#include <iostream>
#include <ios>
#include <iostream>

MBC1::MBC1(Cartridge cartridge, std::vector<uint8_t> rom) {
	this->rom = rom;
	
	this->romBanks = cartridge.romBanks;
	this->ramBanks = cartridge.ramBanks;
	
	//eram.resize(static_cast<size_t>(cartridge.romSize) * 1024);
	eram.resize(static_cast<size_t>((cartridge.ramSize + 1) * 6) * 1024);
}

uint8_t MBC1::fetch8(uint16_t address) {
	if(address < 0x4000) { // bank 0 (fixed)
		uint8_t bank = bankingMode ? (curRomBank & 0xE0) : 0;
		
		uint16_t addr = address | static_cast<uint16_t>(bank * 0x4000);
		
		return rom[addr];
	} else if(address < 0x8000) {
		size_t bank = curRomBank;
		
		if(bank == 0)
			bank = 1;
		
		size_t addr = (bank * 0x4000) | (static_cast<size_t>(address) & 0x3FFF);		
		return rom[addr];
	} else if (address >= 0xA000 && address < 0xBFFF) {
		if(!ramEnabled)
			return 0xFF;
		
		uint8_t bank = bankingMode ? curRamBank : 0;
		
		return eram[(bank * 0x2000) | (address & 0x1FFF)];
	}
	
	return 0xF;
}

void MBC1::write8(uint16_t address, uint8_t data) {
	if(address <= 0x1FFF) {
		ramEnabled = (data & 0xF) == 0xA;
	} else if(address <= 0x3FFF) {
		uint8_t lowerBits = (data & 0x1F);
		
		if(lowerBits == 0)
			lowerBits = 1;
		
		curRomBank = ((curRomBank & 0x60) | lowerBits) % romBanks;
	} else if(address >= 0x4000 && address <= 0x5FFF) {
		if(romBanks > 0x20) {
			uint16_t upperBits = (data & 0x03) % (romBanks >> 5);
			curRomBank = (curRomBank & 0x1F) | (upperBits << 5);
		}
		
		if(ramBanks > 1) {
			curRamBank = data & 0x03;
		}
	} else if(address >= 0x6000 && address <= 0x7FFF) {
		bankingMode = data & 0x01;
	} else if(address >= 0xA000 && address <= 0xBFFF) {
		if(!ramEnabled) {
			return;
		}
		
		uint8_t bank = bankingMode ? (curRamBank) : 0;
		
		eram[(address & 0x1FFF) + (bank * 0x2000)] = data;
		
		ramUpdatd = true;
	}
}