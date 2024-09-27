#pragma once
#include <cstdint>
#include <cmath>
#include <ctime>
#include <iostream>
#include <string>

// https://en.wikipedia.org/wiki/Pulse-width_modulation

struct PulseChannel {
	uint8_t sample(uint32_t cycles) {
		// https://en.wikipedia.org/wiki/Pulse-width_modulation
		
		if (!enabled || lengthTimer == 0) {
			return 0;
		}
		
		uint16_t period = (periodHigh << 8) | periodLow;
		
		if (period == 0) {
			return 0;
		}
		
		uint16_t positionInWave = cycles % period;
		
		// Visual here: https://gbdev.io/pandocs/Audio_Registers.html#ff11--nr11-channel-1-length-timer--duty-cycle
		uint8_t dutyCycle = 0;
		switch (waveDuty) {
			case 0: dutyCycle = 1; break;  // 12.5% duty cycle
			case 1: dutyCycle = 2; break;  // 25% duty cycle
			case 2: dutyCycle = 4; break;  // 50% duty cycle (square wave)
			case 3: dutyCycle = 6; break;  // 75% duty cycle
			default: std::cerr << "Unknown waveDuty! " << std::to_string(waveDuty) << "\n";
		}
		
		// Generate the waveform (high for dutyCycle, low otherwise)
		bool isHigh = positionInWave < (dutyCycle * period / 8);

		// Adjust volume with the envelope
		uint8_t volume = initialVolume;
		if (envDir) {
			volume += sweepPace;  // Volume increases
		} else {
			volume -= sweepPace;  // Volume decreases
		}

		// Return the sample value scaled by volume
		return isHigh ? volume : 0;
	}
	
	// NR10
	uint8_t pace = 0;
	bool direction = false;
	uint8_t individualStep = 0;
	
	// NR11
	uint8_t waveDuty = 0;
	uint8_t lengthTimer = 0;
	
	// NR12
	uint8_t initialVolume = 0;
	bool envDir = 0;
	uint8_t sweepPace = 0;
	
	// NR13
	uint8_t periodLow = 0;
	
	// NR14
	bool trigger;
	bool lengthEnable;
	uint8_t periodHigh;
	
	bool enabled = false;
	bool left = false;
	bool right = false;
};
