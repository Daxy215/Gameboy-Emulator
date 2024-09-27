#pragma once
#include <cstdint>
#include <ctime>
#include <vector>

struct WaveChannel {
	WaveChannel() : waveform(32) {
		
	}
	
	uint8_t sample(uint32_t cycles) {
		/*if(!enabled)
			return 0;
		
		float smapleRate = 1.0f / frequency;
		uint8_t samplePos = (uint8_t)(cycles / smapleRate) % waveform.size();
		
		// 4-bit sample to float
		uint8_t sample = waveform[samplePos] % 0xF;
		return volume * (sample / 15.0f);*/
		return 0;
	}
	
	/*uint8_t frequency = 0;
	uint8_t volume = 0;*/

	// NR30
	bool DAC = false;
	
	// NR31
	uint8_t lengthTimer = 0;
	
	// NR 32
	/**
	 * 0  - Mute
	 * 01 - 100% Volume (Normal use of WAVE Ram)
	 * 10 -  50% Volume (Shifts samples read from WAVE Ram right once)
	 * 11 -  25% Volume (Shifts samples read from WAVE Ram right twice)
	 */
	uint8_t outputLevel = 0;
	
	// NR 33
	uint8_t periodLow = 0;
	
	// NR 34
	bool trigger = false;
	bool lengthEnable = false;
	uint8_t periodHigh = 0;
	
	bool enabled = false;
	bool left = false;
	bool right = false;
	
	std::vector<uint8_t> waveform;
};
