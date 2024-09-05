#include "InterrupHandler.h"

uint8_t InterruptHandler::fetch8(uint16_t address) {
	if(address == 0xFFFF) {
		/**
		 * https://gbdev.io/pandocs/Interrupts.html#ffff--ie-interrupt-enable
		 *
		 * IE: Interrup enable
		 */
	}

	return 0;
}

void InterruptHandler::write8(uint16_t address, uint8_t data) {
	
}
