#pragma once
#include <cstdint>

// https://en.wikipedia.org/wiki/Pulse-width_modulation

// TODO; Convert this to a class

struct PulseChannel {
	uint8_t sample(float cycles) {
		// TODO; Not sure if it should still,
		// update the channel?
		//if (!enabled) {
		//	return 0;
		//}
		
		ticks += cycles;
		
		while(ticks >= period) {
			ticks -= period;
			
			//sweepFrequency = (periodHigh << 8) | periodLow;
			//period         = (2048 - sweepFrequency) * 4;

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
		
		// TODO; Convert to a value between [-1.0, 1.0]
		uint8_t output = (isHigh * (currentVolume));
		return (output) * enabled;
    }
	
    void updateTrigger() {
        enabled = true;
		trigger = false;
		
		// When the length timer reaches 64 (CH1, CH2, and CH4) the channel is turned off.
		// If length timer expired it is reset.
        if (lengthTimer == 0) {
            lengthTimer = 64 - initialTimer;
        }
		
		sweepFrequency = (periodHigh << 8) | periodLow;
		period = (2048 - sweepFrequency) * 4;
		
        currentVolume = initialVolume;
        envelopeCounter = envelopePace;
		
		if(sweepPace > 0) {
			sweepCounter = sweepPace;
		} else {
			sweepCounter = 8;
		}

		if(sweepPace != 0 || individualStep != 0)
			sweepEnabled = true;

		// If the individual step is non-zero,
		// frequency calculation and overflow check are performed immediately.
		if(individualStep != 0) {
			sweepFrequency = calculateFrequency();
			period = (2048 - sweepFrequency) * 4;
		}
    }
	
    void updateSweep() {
		if(sweepCounter > 0) {
			if(sweepPace > 0) {
				sweepCounter = sweepPace;
			} else {
				sweepCounter = 8;
			}

			if(sweepEnabled && sweepPace > 0) {
				auto frequency = calculateFrequency();

				if(frequency <= 2047 && individualStep > 0) {
					sweepFrequency = frequency;

					// Overflow check again
					calculateFrequency();
				}
			}
		} else {
			sweepCounter--;
		}
    }

	uint16_t calculateFrequency() {
		uint16_t frequency = sweepFrequency >> individualStep;
		
		// Subreaction
		if(swpDirection) {
			frequency = sweepFrequency - frequency;
		} else {
			// Increase
			frequency = sweepFrequency + frequency;
		}

		// Check for overflow
		if(frequency > 2047) {
			enabled = false;
		}

		return frequency;
	}
	
    void updateEnvelope() {
		if(envelopePace == 0)
			return;
		
		if(envelopeCounter > 0) {
			envelopeCounter = envelopePace;

			if (envDir && currentVolume < 15) {
                currentVolume++;
        	} else if (!envDir && currentVolume > 0) {
                currentVolume--;
            }
		} else {
			envelopeCounter--;
		}
    }
	
	/**
	 * Just shuts off this channel,
	 * once the timer hits 64
	 * TODO; Check this?
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
		// NR10
		sweepPace = 0;
		swpDirection = false;
		individualStep = 0;
		
		// NR11
		waveDuty = 0;
		//initialTimer = lengthTimer = 0;
		
		// NR12
		initialVolume = 0;
		envDir = false;
		envelopePace = 0;
		
		// NR13
		periodLow = 0;
		
		// NR14
		trigger = false;
		lengthEnable = false;
		periodHigh = 0;
		
		//enabled = false;
		left = false;
		right = false;
		
		sweepFrequency = (periodHigh << 8) | periodLow;
		period          = (2048 - sweepFrequency) * 4;
		
		//  Except for pulse channels, whose phase position is only ever reset by turning the APU off.
		//  This is usually not noticeable unless changing the duty cycle mid-note.
		sequencePointer = 0;
	}
	
	// NR10
	uint8_t sweepPace = 0;
	bool swpDirection = false;
	uint8_t individualStep = 0;
	
	// NR11
	uint8_t waveDuty = 0;
	uint8_t initialTimer = 0;
	int8_t lengthTimer = 0;
	
	// NR12
	uint8_t initialVolume = 0;
	bool envDir = false;
	uint8_t envelopePace = 0;
	
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
	
	uint32_t period = 0;
	
	int8_t sweepCounter = 0;
	bool sweepEnabled = false;

	uint16_t sweepFrequency = 0;
	
	bool enabled = false;
	bool left = false;
	bool right = false;
};
