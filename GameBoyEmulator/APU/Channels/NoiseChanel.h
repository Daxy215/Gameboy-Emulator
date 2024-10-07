#pragma once
#include <cstdint>

struct NoiseChanel {
	uint8_t sample(uint32_t cycles) {
		/*if(!enabled)
			return 0;
		
		// Shift the LFSR
		bool bit0 = lfsr & 1;
		bool bit1 = (lfsr >> 1) & 1;
		bool bit = bit0 ^ bit1;
		
		lfsr = (lfsr >> 1) | (bit << 14);
		
		return (bit0 ? volume : -volume);*/
		
		return 0;
	}

	void reset() {
		// NR41
		lengthTimer = 0;
		
		// NR42
		initialVolume = 0;
		envDir = 0;
		sweepPace = 0;
		
		// NR43
		clockShift = 0;
		lsfrWidth = 0;
		clockDivider = 0;
		
		// NR44
		trigger = false;
		lengthEnable = false;
		
		enabled = false;
		left = false;
		right = false;
	}
	
	// NR41
	uint8_t lengthTimer = 0;
	
	// NR42
	uint8_t initialVolume = 0;
	bool envDir = 0;
	uint8_t sweepPace = 0;
	
	// NR43
	uint8_t clockShift = 0;
	
	/**
	 * 0 = 15-bit
	 * 1 = 7-bit
	 */
	bool lsfrWidth = false;
	
	/**
	 * if clockDivider = 0,
	 * then it's treated as 0.5.
	 */
	uint8_t clockDivider = 0;
	
	// NR44
	bool trigger = false;
	bool lengthEnable = false;

	// Setting
	bool enabled = false;
	bool left = false;
	bool right = false;
};
