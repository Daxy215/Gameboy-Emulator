#pragma once
#include <cstdint>

// https://gbdev.io/pandocs/OAM.html

class OAM {
public:
	uint8_t fetch8(uint16_t address);
	void write8(uint16_t address, uint8_t data);
private:
	uint8_t RAM[0x2000] = { 0 };
};
