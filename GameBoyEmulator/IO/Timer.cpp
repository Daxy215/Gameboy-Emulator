#include "Timer.h"

#include <cstdio>
#include <iostream>

void Timer::tick(uint16_t cycles) {
	divTimer += cycles;
	
	/**
	 * https://gbdev.io/pandocs/Timer_and_Divider_Registers.html#ff04--div-divider-register
	 * 16384Hz which is 256T cycles
	 */
	while (divTimer >= 256) {
		divider++;
		divTimer -= 256;
	}
	
	if(enabled) {
		counterTimer += cycles;
		
		while (counterTimer >= clockSpeed) {
			counter++;
			
			/**
			 * According to https://gbdev.io/pandocs/Timer_and_Divider_Registers.html#ff05--tima-timer-counter
			 *
			 *  When the value overflows (exceeds $FF),
			 *  it is reset to the value specified in TMA (FF06),
			 *  and an interrupt is requested
			 */
			
			// Overflow
			// TODO; Check if this is correct
			if(counter == 0) {
				counter = modulo;
				interrupt |= 0x04; // Timer interrupt
			}
			
			counterTimer -= clockSpeed;
		}
	}
}

uint8_t Timer::fetch8(uint16_t address) {
	if(address == 0xFF04) {
		// https://gbdev.io/pandocs/Timer_and_Divider_Registers.html#ff04--div-divider-register
		return divider;
	} else if(address == 0xFF05) {
		// https://gbdev.io/pandocs/Timer_and_Divider_Registers.html#ff05--tima-timer-counter
		return counter;
	} else if(address == 0xFF06) {
		// https://gbdev.io/pandocs/Timer_and_Divider_Registers.html#ff06--tma-timer-modulo
		return modulo;
	} else if(address == 0xFF07) {
		// https://gbdev.io/pandocs/Timer_and_Divider_Registers.html#ff07--tac-timer-control
		uint8_t result = 0xF8; // 0b11111000
		
		if(enabled) {
			result |= 0x4; // 0b0100
		}
		
		result |= clockSelected;
		
		return result;
	}
}

void Timer::write8(uint16_t address, uint8_t data) {
	if(address == 0xFF04) {
		// https://gbdev.io/pandocs/Timer_and_Divider_Registers.html#ff04--div-divider-register
		// As mentioned writing any value to this address resets it to 0
		divider = 0;
		counterTimer = 0;
	} else if(address == 0xFF05) {
		// https://gbdev.io/pandocs/Timer_and_Divider_Registers.html#ff05--tima-timer-counter
		counter = data;
	} else if(address == 0xFF06) {
		// https://gbdev.io/pandocs/Timer_and_Divider_Registers.html#ff06--tma-timer-modulo
		modulo = data;
	} else if(address == 0xFF07) {
		// https://gbdev.io/pandocs/Timer_and_Divider_Registers.html#ff07--tac-timer-control
		control = data;
		
		enabled = data & 0b0100; // 3rd bit
		
		// I wrote '0x011' instead of '0b0011' :)
		clockSelected = data & 0b0011; // 1st & 2nd bit
		
		// 1 M = 4 T
		switch (clockSelected) {
		case 0:
			clockSpeed = 256 * 4;
			break;
		case 1:
			clockSpeed = 4 * 4;
			break;
		case 2:
			clockSpeed = 16 * 4;
			break;
		case 3:
			clockSpeed = 64 * 4;
			break;
		default:
			clockSpeed = 256 * 4;
			printf("Unknown clock speed of %d\n", clockSelected);
			break;
		}
	}
	
	//printf("Time write; %x -> %x\n", clockSpeed, clockSelected);
	//std::cerr << "";
}
