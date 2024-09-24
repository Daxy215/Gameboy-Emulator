#pragma once
#include <cstdint>
#include <cmath>
#include <ctime>

// https://en.wikipedia.org/wiki/Pulse-width_modulation

struct PulseChannel {
	uint8_t sample(uint8_t cycles) {
		if(!enabled)
			return 0;
		
		float period = 1.0f / frequency;
		double t = fmod(cycles, period);
		
		float dutyThreshold = dutyCycle * period;
		
		return t < dutyCycle ? volume : -volume;
	}
	
	uint8_t frequency = 0;
	uint8_t dutyCycle = 0;
	uint8_t volume = 0;
	
	bool enabled = false;
};
