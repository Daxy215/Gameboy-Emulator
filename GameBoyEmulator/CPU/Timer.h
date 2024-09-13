#pragma once
#include <cstdint>

// https://gbdev.io/pandocs/Timer_and_Divider_Registers.html#timer-and-divider-registers

class Timer {
public:
	void tick(uint16_t cycles);
	
	uint8_t fetch8(uint16_t address);
	void write8(uint16_t address, uint8_t data);

public:
	// To send an interrupt if it occours
	uint8_t interrupt;
	
private:
	uint8_t divider;
	uint8_t counter;
	uint8_t modulo;
	uint8_t control;
	
	// Timers
	uint8_t divTimer;
	uint8_t counterTimer;
	
	bool enabled;
	uint8_t clockSelected;
	uint32_t clockSpeed;
};
