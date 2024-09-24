#include "APU.h"

void APU::tick() {
	
}

uint8_t APU::fetch8(uint16_t address) {
	return 0;
}

void APU::write8(uint16_t address, uint8_t data) {
	if(address == 0xFF10) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff10--nr10-channel-1-sweep
		
	}
}
