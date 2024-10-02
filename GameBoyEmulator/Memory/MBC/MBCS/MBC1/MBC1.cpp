#include "MBC1.h"

MBC1::MBC1(Cartridge cartridge, std::vector<uint8_t> rom) {
	this->rom = rom;
	
	this->romBanks = cartridge.romBanks;
	this->ramBanks = cartridge.ramSize;
	
	//eram.resize(static_cast<size_t>(cartridge.romSize) * 1024);
	eram.resize(static_cast<size_t>((cartridge.ramSize + 1) * 4) * 1024);
}

uint8_t MBC1::fetch8(uint16_t address) {
	if(address < 0x4000) { // bank 0 (fixed)
		uint8_t bank = bankingMode ? static_cast<uint8_t>(curRamBank << 5) % romBanks : 0;
		uint16_t addr = address + static_cast<uint16_t>(bank * 0x4000);
		
		return rom[addr];
	} else if(/*address > 0x4000 && */address < 0x8000) {
		uint8_t bank = static_cast<uint8_t>((curRamBank << 5) | curRomBank) % romBanks;
		
		if(bank == 0)
			bank = 1;
		
		uint16_t addr = (address - 0x4000) + (bank * 0x4000);
		
		return rom[addr];
	} else if (address >= 0xA000 && address < 0xBFFF) {
		// Switchable RAM bank
		if (!ramEnabled) {
			return 0xFF;
		}
		
		uint8_t bank = bankingMode ? (curRamBank % ramBanks) : 0;
		
		return eram[(address - 0xA000) + (bank * 0x2000)];
	}
	
	return 0xFF;
}

void MBC1::write8(uint16_t address, uint8_t data) {
	// RAM enabling (0x0000 - 0x1FFF)
	if (address <= 0x1FFF) {
		/**
		 * According to;
		 * https://gbdev.io/pandocs/MBC1.html
		 *
		 * Before external RAM can be read or written, it must be enabled by writing $A to anywhere in this address space.
		 * Any value with $A in the lower 4 bits enables the RAM attached to the MBC, and any other value disables the RAM.
		 * It is unknown why $A is the value used to enable RAM.
		 */
		
		ramEnabled = (data & 0x0F) == 0xA;
	}
	// ROM bank switching (0x2000 - 0x3FFF)
	else if (address >= 0x2000 && address <= 0x3FFF) {
		data &= 0x1F;
		
		if(data == 0)
			data = 1;
		
		/*if(data == 0x20) { curRomBank = 0x21; return; }
		if(data == 0x40) { curRomBank = 0x41; return; }
		if(data == 0x60) { curRomBank = 0x61; return; }*/
		
		curRomBank = data;
	}
	// ROM or RAM bank switching (0x4000 - 0x5FFF)
	else if (address >= 0x4000 && address <= 0x5FFF) {
		if (!bankingMode) {
			// ROM bank switching (high bits)
			curRomBank &= 0x1F; // Clear upper bits
			curRomBank |= ((data & 0x3) << 5); // Set upper 2 bits (5 and 6)
		} else {
			curRamBank = data & 0x3;
		}
	}
	// ROM/RAM mode selection (0x6000 - 0x7FFF)
	else if (address >= 0x6000 && address <= 0x7FFF) {
		bankingMode = data & 0x1;
	} else if(address >= 0xA000 && address <= 0xBFFF) {
		if(!ramEnabled) {
			return;
		}
		
		uint8_t bank = (bankingMode ? (curRamBank % ramBanks) : 0);
		
		uint16_t addr = address - 0xA000;
		eram[addr + (bank * 0x2000)] = data;
	}
}
