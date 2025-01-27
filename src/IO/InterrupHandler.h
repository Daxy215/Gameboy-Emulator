#pragma once

// https://gbdev.io/pandocs/Interrupts.html

class CPU;

class InterruptHandler {
public:
	InterruptHandler() = default;
	
	uint8_t handleInterrupt(CPU& cpu);
	
	uint8_t fetch8(uint16_t address);
	void write8(uint16_t address, uint8_t data);
	
public:
	/**
	 * Interrupt Master Enable. A flag that is used to determine,
	 * whether interrupts can occur.
	 */
	bool IME = true;
	
	uint8_t IE = 0;
	uint8_t IF = 0;
};
