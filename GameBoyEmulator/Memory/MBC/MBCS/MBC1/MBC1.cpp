#include "MBC1.h"

MBC1::MBC1(Cartridge cartridge, std::vector<uint8_t> rom) {
	this->rom = rom;
	
	this->romSize = cartridge.romSize;
	this->ramSize = cartridge.ramSize;
	
	//eram.resize(static_cast<size_t>(cartridge.romSize) * 1024);
	eram.resize(static_cast<size_t>(cartridge.ramSize * 2) * 1024);
}

uint8_t MBC1::fetch8(uint16_t address) {
	if(address <= 0x3FFF) { // bank 0 (fixed)
		uint8_t bank = (bankingMode ? static_cast<uint8_t>(curRomBank << 5) : 0);
		uint16_t addr = address | static_cast<uint16_t>(bank * 0x4000);
		
		return rom[addr];
	} else if(/*address > 0x4000 && */address < 0x7FFF) {
		// Switchable ROM bank
		// bank 0 isn't allowed in this region
		//uint8_t bank = (curRomBank == 0) ? 1 : curRomBank;
		uint8_t bank = (curRomBank & 0x1F);
		
		if(bank == 0)
			bank = 1;
		
		bank |= static_cast<uint8_t>((curRamBank & 0xC0) << 5);
		
		uint16_t addr = (address - 0x4000) + (bank * 0x4000);
		
		return rom[addr];
	} else if (address >= 0xA000 && address < 0xBFFF) {
		// Switchable RAM bank
		if (!ramEnabled) {
			return 0xFF;
		}
		
		uint8_t bank = (curRamBank & 0x1F);
		
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
		curRomBank = data & 0x1F;
		
		if(data == 0xE1) {
			curRomBank = 1;	
		}
		
		/**
		 * According to: https://gbdev.io/pandocs/MBC1.html#20003fff--rom-bank-number-write-only
		 *
		 * If the ROM Bank number is set to a higher value,
		 * than the number of banks inm the cart,
		 * the bank number is maske to the required number of bits.
		 */
		uint8_t numBanks = static_cast<uint8_t>(log2(static_cast<double>(romSize) / 16));
		
		if(curRomBank > numBanks) {
			uint8_t bitMask = (1 << numBanks) - 1;
			curRomBank &= bitMask;
		}
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
		
		uint8_t bank = (curRamBank & 0x1F);
		
		uint16_t addr = address - 0xA000;
		eram[addr + (bank * 0x2000)] = data;
	}
}
