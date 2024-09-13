#include <cstdint>
#include <cstdio>

#include "InterrupHandler.h"

void InterruptHandler::handleInterrupt() {
	if(IME) {
		
	}
}

uint8_t InterruptHandler::fetch8(uint16_t address) {
	if(address == 0xFF0F)
		return IF;
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
