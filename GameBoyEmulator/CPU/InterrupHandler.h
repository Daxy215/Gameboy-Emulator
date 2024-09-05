#pragma once
#include <cstdint>

// https://gbdev.io/pandocs/Interrupts.html

class InterruptHandler {
public:
	InterruptHandler() = default;
	
	uint8_t fetch8(uint16_t address);
	void write8(uint16_t address, uint8_t data);
public:
	/**
	 * Interrupt Master Enable. A flag that is used to determine,
	 * whether interrupts can occur.
	 */
	bool IME;
};
