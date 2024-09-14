#pragma once
#include <cstdint>

class Joypad {
public:
	uint8_t fetch8(uint16_t address);
	void write8(uint16_t address, uint8_t data);
};
