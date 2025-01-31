#pragma once

#include <assert.h>
#include <cstdint>
#include <vector>

struct WaveChannel {
	WaveChannel() : waveform(16) {
		
	}
	
	uint8_t sample(uint32_t cycles) {
		// TODO; What is DAC?
		
		if(!enabled/* || !DAC*/)
			return 0;
		
		ticks += cycles;
		
		while(ticks >= period) {
			ticks = 0;
			
			period          = (2048 - sweepFrequency) * 4;

			sequencePointer = (sequencePointer + 1) % 32;
		}
		
		uint8_t sampleIndex = sequencePointer / 2;
		uint8_t wave = waveform[sampleIndex];
		uint8_t sample;
		
		if (sequencePointer % 2 == 0) {
			// Upper nibble
			sample = (wave >> 4) & 0x0F;
		} else {
			// Lower nibble
			sample = wave & 0x0F;
		}
		
		/**
		 * 0  - Mute
		 * 01 - 100% Volume (Normal use of WAVE Ram)
		 * 10 -  50% Volume (Shifts samples read from WAVE Ram right once)
		 * 11 -  25% Volume (Shifts samples read from WAVE Ram right twice)
		 */
		switch(outputLevel) {
			case 0: {
				sample = 0;
				break;
			}
			case 1: {
				break;
			}
			case 2: {
				sample = sample >> 1;
				break;
			}
			case 3: {
				sample = sample >> 2;
				break;
			}
			default: {
				assert(false);
				break;
			}
		}
		
		return ((sample) & 0x0F);
	}
	
	void updateTrigger() {
		enabled = true;
		trigger = false;
		
		// When the length timer reaches 256 (CH3), the channel is turned off.
		// If the length timer expired it is reset.
		/*if (lengthTimer >= 256) {
			lengthTimer = 0;
		}*/

        if (lengthTimer == 0) {
			lengthTimer = 256 - initialTimer;
		}
		
		sweepFrequency = (periodHigh << 8) | periodLow;
		sequencePointer = 0;
		
		period = (2048 - sweepFrequency) * 4;
	}
	
	/**
	 * Just shuts off this channel,
	 * once the timer hits 256?
	 * TODO; Double check this
	 */
	void updateCounter() {
		if (lengthEnable) {
			lengthTimer--;
			
			if(lengthTimer <= 0) {
				enabled = false;
			}
		}
	}
	
	void reset() {
		// NR30
		DAC = false;
		
		// NR31
		//initialTimer = lengthTimer = 0;
		
		// NR32
		outputLevel = 0;
		
		// NR 33
		periodLow = 0;
		
		// NR34
		trigger = false;
		lengthEnable = false;
		periodHigh = 0;
		
		enabled = false;
		left = false;
		right = false;
		
		sweepFrequency = (periodHigh << 8) | periodLow;
		period          = (2048 - sweepFrequency) * 4;
		sequencePointer = 1;
		
		// TODO; Clear waveform?
	}
	
	// NR30
	bool DAC = false;
	
	// NR31
	uint8_t initialTimer = 0;
	int16_t lengthTimer = 0;
	
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
	
	uint16_t sweepFrequency = 0;
	
	uint32_t period = 0;
	uint8_t sequencePointer = 0;
	uint32_t ticks = 0;
	
	bool enabled = false;
	bool left = false;
	bool right = false;
	
	std::vector<uint8_t> waveform;
};
