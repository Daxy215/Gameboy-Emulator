#pragma once
#include <cstdint>

// https://en.wikipedia.org/wiki/Pulse-width_modulation

// TODO; Convert this to a class

struct PulseChannel {
	uint8_t sample(uint32_t cycles) {
		if (!enabled || (lengthEnable && lengthTimer == 0)) {
			return 0;
		}

		if (period == 0) {
			return 0;
		}
		
		ticks += cycles;
		
		while(ticks >= period) {
			ticks -= period;
			sequencePointer = (sequencePointer + 1) % 8;
		}
		
		// Visual here: https://gbdev.io/pandocs/Audio_Registers.html#ff11--nr11-channel-1-length-timer--duty-cycle
		
		static const uint8_t dutyCycles[4][8] = {
			{0, 0, 0, 0, 0, 0, 0, 1},  // 12.5% duty cycle
			{0, 0, 0, 0, 0, 0, 1, 1},  // 25% duty cycle
			{0, 0, 0, 0, 1, 1, 1, 1},  // 50% duty cycle
			{0, 1, 1, 1, 1, 1, 1, 1}   // 75% duty cycle
		};
		
		bool isHigh = dutyCycles[waveDuty][sequencePointer];
		
		return isHigh ? 1 : 0;
    }
	
    void updateTrigger() {
		/*if(enabled)
			return;*/
		
        enabled = true;
		
        if (lengthTimer == 0) {
            lengthTimer = 64;  // Reload length timer (max 64 for pulse channels)
        }
		
        currentVolume = initialVolume;
        envelopeCounter = sweepPace;
		
        /*uint16_t frequency = (periodHigh << 8) | periodLow;
        period = (2048 - frequency) * 4;*/
		
        sequencePointer = 0;
        trigger = false;
    }
	
    void updateSweep() {
        if (/*sweepPace > 0 && */sweepCounter <= 0) {
            //uint16_t frequency = (periodHigh << 8) | periodLow;
            uint16_t delta = sweepFrequency >> individualStep;
        	
            if (direction) {
                sweepFrequency -= delta;
            } else {
                sweepFrequency += delta;
            }
			
            if (sweepFrequency > 2047) {
                enabled = false;
            } else {
                /*periodLow = frequency & 0xFF;
                periodHigh = (frequency >> 8) & 0xFF;
				
            	period = (periodHigh << 8) | periodLow;*/
            	period  = (2048 - sweepFrequency) * 4;
            }
			
            sweepCounter = sweepPace;
        } else {
            sweepCounter--;
        }
    }
	
    void updateEnvelope() {
        if (sweepPace > 0 && envelopeCounter == 0) {
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
	uint16_t ticks = 0;
	
	// NR13
	uint8_t periodLow = 0;
	
	// NR14
	bool trigger = false;
	bool lengthEnable = false;
	uint8_t periodHigh = 0;
	
	int32_t period = 0;
	
	uint8_t sweepCounter = 0;
	uint16_t sweepFrequency = 0;
	
	bool enabled = false;
	bool left = false;
	bool right = false;
};
