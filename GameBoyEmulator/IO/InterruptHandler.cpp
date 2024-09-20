#include <cstdint>

#include "../CPU/CPU.h"
#include "InterrupHandler.h"

uint8_t InterruptHandler::handleInterrupt(CPU& cpu) {
	if(cpu.interruptHandler.IME || cpu.halted) {
		uint8_t interrupt = cpu.interruptHandler.IF & cpu.interruptHandler.IE;
		
		if(interrupt == 0)
			return 0;
		
		cpu.halted = false;
		if(cpu.interruptHandler.IME == false) {
			return 0;
		}
		
		cpu.interruptHandler.IME = false;
		
		cpu.pushToStack(cpu.PC);
		
		// https://gbdev.io/pandocs/Interrupt_Sources.html
		if(interrupt & 0x01) { // V-Blank interrupt
			cpu.PC = 0x0040;
		} else if(interrupt & 0x02) { // LCD STAT interrupt
			cpu.PC = 0x0048;
		} else if(interrupt & 0x04) { // Timer interrupt
			cpu.PC = 0x0050;
		} else if(interrupt & 0x08) { // Serial interrupt
			cpu.PC = 0x0058;
		} else if(interrupt & 0x10) { // Joypad interrupt
			cpu.PC = 0x0060;
		}
		
		cpu.interruptHandler.IF &= ~interrupt;
		
		return 4;
	}
	
	return 0;
}

uint8_t InterruptHandler::fetch8(uint16_t address) {
	if(address == 0xFF0F)
		return IF; // | 0xE0?
	else if(address == 0xFFFF) {
		/**
		 * https://gbdev.io/pandocs/Interrupts.html#ffff--ie-interrupt-enable
		 *
		 * IE: Interrup enable
		 */
		
		return IE;
	}
	
	return 0;
}

void InterruptHandler::write8(uint16_t address, uint8_t data) {
	if(address == 0xFF0F) {
		IF = data;
	} else if(address == 0xFFFF) {
 		IE = data;
	}
}
