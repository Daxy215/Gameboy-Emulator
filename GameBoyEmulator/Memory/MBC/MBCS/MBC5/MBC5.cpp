#include "MBD5.h"

MBC5::MBC5(Cartridge cartridge, std::vector<uint8_t> rom) {
	this->rom = rom;
	
	this->romBanks = cartridge.romBanks;
	this->ramBanks = cartridge.ramBanks;
	
	//eram.resize(static_cast<size_t>(cartridge.romSize) * 1024);
	eram.resize(static_cast<size_t>((cartridge.ramSize + 1) * 4) * 1024);
}

uint8_t MBC5::fetch8(uint16_t address) {
	if (address >= 0xA000 && address < 0xBFFF) {
		if(!ramEnabled)
			return 0xFF;
		
		uint8_t bank = curRamBank;
		
		return eram[(bank * 0x2000) | (address & 0x1FFF)];
	} else {
		size_t bank;
		
		if(address < 0x4000) {
			bank = 0;
		} else {
			bank = curRomBank;
		}
		
		size_t addr = (bank * 0x4000) | (static_cast<size_t>(address) & 0x3FFF);
		return rom[addr];
	}
}

void MBC5::write8(uint16_t address, uint8_t data) {
	if(address < 0x1FFF) {
		ramEnabled = (data & 0xF) == 0xA;
	} else if(address <= 0x2FFF) {
		curRomBank = ((curRomBank & 0x100) | (data)) % romBanks;
	} else if(address <= 0x3FFF) {
		curRomBank = ((curRomBank & 0x0FF) | (data & 0x1) << 8) % romBanks;
	} else if(address >= 0x4000 && address < 0x5FFF) {
		curRomBank = ((data & 0x0F)) % ramBanks;
	} else if(address >= 0x6000 && address < 0x7FFF) {
		
	} else if(address >= 0xA000 && address <= 0xBFFF) {
		if(!ramEnabled) {
			return;
		}
		
		uint8_t bank = curRamBank;
		
		eram[(address & 0x1FFF) + (bank * 0x2000)] = data;
	}
}