#pragma once
#include <cstdint>

struct NoiseChanel {
	uint8_t sample(uint8_t cycles) {
		if(!enabled)
			return 0;
		
		// Shift the LFSR
		bool bit0 = lfsr & 1;
		bool bit1 = (lfsr >> 1) & 1;
		bool bit = bit0 ^ bit1;

		lfsr = (lfsr >> 1) | (bit << 14);

		return (bit0 ? volume : -volume);
	}
	
	uint8_t volume;
	
	// Linear Feedback Shift Register
	uint16_t lfsr;
	
	bool enabled;
};
