#include "MBCS/MBC0/MBC0.h"
#include "MBCS/MBC1/MBC1.h"

MBC::MBC() {
	
}

MBC::MBC(Cartridge cartridge, std::vector<uint8_t> rom) {
	switch (cartridge.type) {
	case enums::ROM_ONLY:
		curMBC = std::make_unique<MBC0>(cartridge, rom);
		
		break;
	case enums::MBC1:
		curMBC = std::make_unique<MBC1>(cartridge, rom);
		
		break;
	}
}

uint8_t MBC::read(uint16_t address) {
	return curMBC->fetch8(address);
}

void MBC::write(uint16_t address, uint8_t data) {
	curMBC->write8(address, data);
}

uint8_t MBC::fetch8(uint16_t address) {
	return 0;
}

void MBC::write8(uint16_t address, uint8_t data) {
	
}
