#include "OAM.h"

uint8_t OAM::fetch8(uint16_t address) {
	return RAM[address - 0xFE00];
}

void OAM::write8(uint16_t address, uint8_t data) {
	RAM[address] = data;
}
