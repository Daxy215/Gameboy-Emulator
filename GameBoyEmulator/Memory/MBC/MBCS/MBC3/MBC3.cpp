#include "MBC3.h"

#include <assert.h>

MBC3::MBC3(Cartridge cartridge, std::vector<uint8_t> rom) {
	this->rom = rom;
	
	this->romBanks = cartridge.romBanks;
	this->ramBanks = cartridge.ramBanks;
	
	//eram.resize(static_cast<size_t>(cartridge.ramSize) * 1024);
	eram.resize(static_cast<size_t>(cartridge.ramSize + 8) * 1024);
}

uint8_t MBC3::fetch8(uint16_t address) {
	if(address <= 0x3FFF) {
		return rom[address];
	} else if(address >= 0x4000 && address <= 0x7FFF) {
		size_t addr = static_cast<size_t>(curRomBank * 0x4000) | (static_cast<size_t>(address - 0x4000));
		assert(addr < rom.size());
		
		return rom[addr];
	} else if (address >= 0xA000 && address < 0xBFFF) {
		size_t addr = (address - 0xA000) | (curRamBank * 0x2000);
		assert(addr <= eram.size());
		
		return eram[addr];
	}
}

void MBC3::write8(uint16_t address, uint8_t data) {
	if(address <= 0x1FFF) {
		ramEnabled = (data & 0xF) == 0xA;
	} else if(address >= 0x2000 && address <= 0x3FFF) {
		uint16_t lowerBits = (data);
		
		if(lowerBits == 0)
			lowerBits = 1;
		
		//curRomBank = ((curRomBank & 0x60) | lowerBits);*/
		curRomBank = lowerBits & 0x7F;
	} else if(address >= 0x4000 && address <= 0x5FFF) {
		if (data <= 0x03) {
			curRamBank = data;
			rtcRegister = false;
		}
		
		if (data >= 0x08 && data <= 0xC) {
			// TODO; RTC
			rtcRegister = true;
		}
	} else if(address >= 0x6000 && address <= 0x7FFF) {
		// TODO; RTC
	} else if(address >= 0xA000 && address <= 0xBFFF) {
		if(!ramEnabled || rtcRegister) {
			return;
		}
		
		size_t addr = (address - 0xA000) + (curRamBank * 0x2000);
		assert(addr < eram.size());
		
		eram[addr] = data;
	}
}
