#pragma once
#include <cstdint>

// https://en.wikipedia.org/wiki/Pulse-width_modulation

// TODO; Convert this to a class

struct PulseChannel {
	uint8_t sample(float cycles) {
		/*if (!enabled/* || (lengthEnable && lengthTimer == 0)#1#) {
			return 0;
		}
		*/
		
		ticks += cycles;
		
		while(ticks >= period) {
			ticks = 0;
			
			period          = (2048 - sweepFrequency) * 4;
			sequencePointer = (sequencePointer + 1) % 8;
		}
		
		// Visual here: https://gbdev.io/pandocs/Audio_Registers.html#ff11--nr11-channel-1-length-timer--duty-cycle
		
		uint8_t dutyCycles[4][8] = {
			{0, 0, 0, 0, 0, 0, 0, 1},
			{1, 0, 0, 0, 0, 0, 0, 1},
			{1, 0, 0, 0, 0, 1, 1, 1},
			{0, 1, 1, 1, 1, 1, 1, 0}
		};
		
		uint8_t isHigh = dutyCycles[waveDuty][sequencePointer];
		
		return (isHigh * (currentVolume)) * enabled;
    }
	
    void updateTrigger() {
        enabled = true;
		trigger = false;
		
        if (lengthTimer == 0) {
            lengthTimer = 64;
        }
		
		period = (2048 - sweepFrequency) * 4;
		
        currentVolume = initialVolume;
        envelopeCounter = sweepPace;
		
		if(envelopeCounter == 0)
			envelopeCounter = 8;
		
        sequencePointer = 0;
    }
	
    void updateSweep() {
        if (sweepPace != 0) {
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
        }
    }
	
    void updateEnvelope() {
        if (envelopeCounter == 0) {
            if (envDir && currentVolume < 15) {
                currentVolume++;
            } else if (!envDir && currentVolume > 0) {
                currentVolume--;
            }
        	
            envelopeCounter = sweepPace;
        	if(envelopeCounter == 0)
        		envelopeCounter = 8;
        } else if (sweepPace > 0) {
            envelopeCounter--;
        }
    }
	
	/**
	 * Just shuts off this channel,
	 * once the timer hits 0
	 */
	void updateCounter() {
        if (lengthEnable && lengthTimer > 0) {
            lengthTimer--;
			
        	if(lengthTimer <= 0) {
        		enabled = false;
        	}
        }
    }
	
	void reset() {
		/*// NR10
		pace = 0;
		direction = false;
		individualStep = 0;
		
		// NR11
		waveDuty = 0;
		lengthTimer = 0;
		
		// NR12
		initialVolume = 0;
		envDir = false;
		sweepPace = 0;
		
		// NR13
		periodLow = 0;
		
		// NR14
		trigger = false;
		lengthEnable = false;
		periodHigh = 0;
		
		enabled = false;
		left = false;
		right = false;*/
	}
	
	const uint32_t GB_CLOCK_SPEED = 4194304;
	
	// NR10
	uint8_t pace = 0;
	bool direction = false;
	uint8_t individualStep = 0;
	
	// NR11
	uint8_t waveDuty = 0;
	int8_t lengthTimer = 0;
	
	// NR12
	uint8_t initialVolume = 0;
	bool envDir = false;
	uint8_t sweepPace = 0;
	
	uint8_t currentVolume = 0;
	uint8_t envelopeCounter = 0;
	uint16_t sequencePointer = 0;
	float ticks = 0;
	
	// NR13
	uint8_t periodLow = 0;
	
	// NR14
	bool trigger = false;
	bool lengthEnable = false;
	uint8_t periodHigh = 0;
	
	uint32_t period = 0;
	
	uint8_t sweepCounter = 0;
	uint16_t sweepFrequency = 0;
	
	bool enabled = false;
	bool left = false;
	bool right = false;
};
