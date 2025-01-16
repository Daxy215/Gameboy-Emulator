#pragma once

#include <cstdint>
#include <vector>

struct WaveChannel {
	WaveChannel() : waveform(16) {
		
	}
	
	uint8_t sample(uint32_t cycles) {
		if(trigger)
			updateTrigger();
		
		// TODO; What is DAC?
		
		if(!enabled || !DAC)
			return 0;
		
		return 0;
		
		/*
		float smapleRate = 1.0f / sweepFrequency;
		uint8_t samplePos = (uint8_t)(cycles / smapleRate) % waveform.size();
		
		uint8_t sample = waveform[samplePos] % 0xF;
		return (sample / 15.0f);*/
	}
	
	void updateTrigger() {
		enabled = true;
		trigger = false;
		
		// When the length timer reaches 256 (CH3), the channel is turned off.
		if (lengthTimer >= 256) {
			lengthTimer = 0;
		}
		
		sweepFrequency = (periodHigh << 8) | periodLow;
		//period = (2048 - sweepFrequency) * 4;
		
		/*currentVolume = initialVolume;
		envelopeCounter = sweepPace;
		
		if(envelopeCounter == 0)
			envelopeCounter = 8;
		
		sequencePointer = 0;*/
	}
	
	void updateSweep() {
		/*if (sweepPace != 0) {
			sweepCounter--;
			
			if(sweepCounter <= 0) {
				sweepCounter = sweepPace;
        		
				if (direction) {
					sweepFrequency += individualStep;
            		
					if (sweepFrequency == 2047) {
						enabled = false;
					}
				} else {
					sweepFrequency -= individualStep;
            		
					if (sweepFrequency == 0) {
						enabled = false;
					}
				}
				
				sweepFrequency &= 2047;
			}
		}*/
	}
	
	/**
	 * Just shuts off this channel,
	 * once the timer hits 256?
	 * TODO; Double check this
	 */
	void updateCounter() {
		if (lengthEnable) {
			lengthTimer++;
			
			if(lengthTimer >= 256) {
				enabled = false;
			}
		}
	}
	
	void reset() {
		// NR30
		DAC = false;
		
		// NR31
		lengthTimer = 0;
		
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
		
		// TODO; Clear waveform?
	}
	
	// NR30
	bool DAC = false;
	
	// NR31
	uint16_t lengthTimer = 0;
	
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
	
	bool enabled = false;
	bool left = false;
	bool right = false;
	
	std::vector<uint8_t> waveform;
};
