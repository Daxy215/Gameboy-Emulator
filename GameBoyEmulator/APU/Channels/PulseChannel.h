#pragma once
#include <cstdint>

// https://en.wikipedia.org/wiki/Pulse-width_modulation

// TODO; Convert this to a class

struct PulseChannel {
	uint8_t sample(uint32_t cycles) {
		if (!enabled/* || (lengthEnable && lengthTimer == 0)*/) {
			return 0;
		}
		
		ticks += cycles;
		
		while(ticks >= period) {
			ticks = 0;
			
			period = (2048 - sweepFrequency)/* * 4*/;
			sequencePointer = (sequencePointer + 1) % 8;
		}
		
		// Visual here: https://gbdev.io/pandocs/Audio_Registers.html#ff11--nr11-channel-1-length-timer--duty-cycle
		
		static const uint8_t dutyCycles[4][8] = {
			{0, 0, 0, 0, 0, 0, 0, 1},  // 12.5% duty cycle
			{1, 0, 0, 0, 0, 0, 0, 1},  // 25% duty cycle
			{1, 0, 0, 0, 0, 1, 1, 1},  // 50% duty cycle
			{0, 1, 1, 1, 1, 1, 1, 0}   // 75% duty cycle
		};
		
		uint8_t isHigh = dutyCycles[waveDuty][sequencePointer];
		
		return isHigh * currentVolume;
    }
	
    void updateTrigger() {
        enabled = true;
		
        if (lengthTimer == 0) {
            lengthTimer = 64;
        }
		
        currentVolume = initialVolume;
        envelopeCounter = sweepPace;
		
        //sequencePointer = 0;
        trigger = false;
    }
	
    void updateSweep() {
        if (sweepCounter <= 0) {
            uint16_t delta = sweepFrequency >> individualStep;
        	
            if (direction) {
                sweepFrequency -= delta;
            } else {
                sweepFrequency += delta;
            }
			
        	//period  = (2048 - sweepFrequency) * 4;
			
        	if (sweepFrequency > 2047) {
                enabled = false;
            }
			
            sweepCounter = sweepPace;
        } else {
            sweepCounter--;
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
        } else if (sweepPace > 0) {
            envelopeCounter--;
        }
    }
	
    void updateCounter() {
        if (lengthEnable && lengthTimer > 0) {
            lengthTimer--;
        }
		
        if (lengthEnable && lengthTimer == 0) {
            enabled = false;
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
	uint8_t lengthTimer = 0;
	
	// NR12
	uint8_t initialVolume = 0;
	bool envDir = false;
	uint8_t sweepPace = 0;
	
	uint8_t currentVolume = 0;
	uint8_t envelopeCounter = 0;
	uint16_t sequencePointer = 0;
	uint32_t ticks = 0;
	
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
