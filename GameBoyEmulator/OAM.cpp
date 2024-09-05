#include "OAM.h"

#include "Bitwise.h"

uint8_t OAM::fetch8(uint16_t address) {
	return RAM[address - 0xFE00];
}

void OAM::write8(uint16_t address, uint8_t data) {
	uint8_t yPos = check_bit(data, 0);
	uint8_t xPos = check_bit(data, 1);
	uint8_t tileIndex = check_bit(data, 2);
	uint8_t flags = check_bit(data, 3);
	
	RAM[address] = data;
}
