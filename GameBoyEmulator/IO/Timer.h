#pragma once
#include <cstdint>

// https://gbdev.io/pandocs/Timer_and_Divider_Registers.html#timer-and-divider-registers

class Timer {
public:
	void tick(uint16_t cycles);
	
	uint8_t fetch8(uint16_t address);
	void write8(uint16_t address, uint8_t data);
	
public:
	// To send an interrupt if it occurs
	uint8_t interrupt = 0;
	
private:
	/**
	 * Bully test rom suggests
	 * that initial divider
	 * should be 0xAD for DMG.
	 */
	uint8_t divider = 0xAD;
	uint8_t counter = 0;
	uint8_t modulo = 0;
	uint8_t control = 0;
	
	// Timers
	uint16_t divTimer = 0;
	uint16_t counterTimer = 0;
	
	bool enabled = false;
	uint8_t clockSelected = 0;
	uint32_t clockSpeed = 256;
};
