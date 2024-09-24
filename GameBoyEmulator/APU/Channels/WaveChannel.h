#pragma once
#include <cstdint>
#include <ctime>
#include <vector>

struct WaveChannel {
	WaveChannel() : frequency(0), volume(0), enabled(false), waveform(32) {
		
	}

	uint8_t sample(uint8_t cycles) {
		if(!enabled)
			return 0;
		
		float smapleRate = 1.0f / frequency;
		uint8_t samplePos = (uint8_t)(cycles / smapleRate) % waveform.size();
		
		// 4-bit sample to float
		uint8_t sample = waveform[samplePos] % 0xF;
		return volume * (sample / 15.0f);
	}
	
	uint8_t frequency;
	uint8_t volume;
	
	bool enabled;
	
	std::vector<uint8_t> waveform;
};
