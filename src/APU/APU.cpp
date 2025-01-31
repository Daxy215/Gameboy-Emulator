#include "APU.h"

#include "../Utility/Bitwise.h"

#include <iostream>

#include "../Memory/Cartridge.h"

/*
uint8_t* APU::audio_chunk;
uint32_t APU::audio_len = 0;
uint8_t* APU::audio_pos;
*/

APU::APU()
	: ch1(), ch2(), ch4(), newSamples(0) {
	
	
}

void APU::init() {
	// https://www.libsdl.org/release/SDL-1.2.15/docs/html/guideaudioexamples.html
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
	}
	
	SDL_AudioSpec want, have;
	
	// Clear the structure
	SDL_memset(&want, 0, sizeof(want));
	want.freq = 44100;
	want.format = /*AUDIO_S16SYS*/AUDIO_U8;
	want.channels = 2;
	want.samples = /*44100 / 60*//*4096*/1024;
	want.callback = fill_audio;
	want.userdata = this;
	
	if (SDL_OpenAudio(&want, NULL) < 0) {
		std::cerr << "Failed to open audio: " << SDL_GetError() << std::endl;
		SDL_Quit();
	}
	
	SDL_PauseAudio(0); // Start audio playback
}

void APU::tick(uint32_t cycles) {
	// APU Disabled
	//if (!enabled) {
	//	return;
	//}
	
	// https://www.reddit.com/r/EmuDev/comments/5gkwi5/comment/dat3zni/
	
	/**
	 * TODO; https://gbdev.io/pandocs/Audio_details.html#div-apu
	 * A “DIV-APU” counter is increased every time DIV’s bit 4 (5 in double-speed mode)
	 * goes from 1 to 0, therefore at a frequency of 512 Hz
	 * (regardless of whether double-speed is active).
	 */
	
	ticks += cycles;
	
	// 512hz -> 8192 T-Cycles
	// 4194304/512 = 8192
	while(ticks >= 8192) {
		ticks -= 8192;
		
		counter = (counter + 1) % 8;
		
		if(counter % 2 == 0) {
			ch1.updateCounter();
			ch2.updateCounter();
			ch3.updateCounter();
			ch4.updateCounter();
		}
		
		// Clock sweep ever 2 and 6 steps
		if(counter == 2 || counter == 6) {
			ch1.updateSweep();
		}
		
		// Clock volume envelopes every 7 steps
		if(counter == 7) {
			ch1.updateEnvelope();
			ch2.updateEnvelope();
			//ch3.updateEnvelope();
			//ch4.updateEnvelope();
		}
	}
}

uint8_t APU::fetch8(uint16_t address) {
	if(address != 0xFF26) {
		printf("");
	}
	
    if (address == 0xFF26) {
        // NR52 - Audio Master Control
        uint8_t result = 0;
    	
        result |= (enabled << 7);      // Bit 7 - Audio on/off
        result |= (ch4.enabled << 3);  // Bit 3 - Channel 4 status
        result |= (ch3.enabled << 2);  // Bit 2 - Channel 3 status
        result |= (ch2.enabled << 1);  // Bit 1 - Channel 2 status
        result |= (ch1.enabled << 0);  // Bit 0 - Channel 1 status
		
		//printf("Fetching channcels; %x %d %d %d %d %d\n", result, enabled, ch4.enabled, ch3.enabled, ch2.enabled, ch1.enabled);

		if(ch1.enabled) {
			//printf("CH1 Length %x = %x ; %d\n", ch1.initalLength, ch1.lengthTimer, ch1.lengthEnable);
		}
    	
        return result;
    } else if (address == 0xFF25) {
        // NR51 - Sound panning
        uint8_t result = 0;
    	
        result |= (ch4.left << 7);   // Bit 7 - Channel 4 Left
        result |= (ch3.left << 6);   // Bit 6 - Channel 3 Left
        result |= (ch2.left << 5);   // Bit 5 - Channel 2 Left
        result |= (ch1.left << 4);   // Bit 4 - Channel 1 Left
        result |= (ch4.right << 3);  // Bit 3 - Channel 4 Right
        result |= (ch3.right << 2);  // Bit 2 - Channel 3 Right
        result |= (ch2.right << 1);  // Bit 1 - Channel 2 Right
        result |= (ch1.right << 0);  // Bit 0 - Channel 1 Right
    	
        return result;
    } else if (address == 0xFF24) {
        // NR50 - Master Volume & VIN Panning
        uint8_t result = 0;
    	
        result |= (vinLeft << 7);              // Bit 7 - VIN Left
        result |= (leftVolume << 4);           // Bits 6-4 - Left Volume
        result |= (vinRight << 3);             // Bit 3 - VIN Right
        result |= (rightVolume & 0b00000111);  // Bits 2-0 - Right Volume
    	
        return result;
    } else if (address == 0xFF10) {
        // NR10 - Channel 1 Sweep
        uint8_t result = 0;
    	
        result |= (1 << 7);                // Bits 7   - Always set
        result |= (ch1.sweepPace << 4);    // Bits 6-4 - Sweep pace
        result |= (ch1.swpDirection << 3); // Bit  3   - Sweep direction
        result |= (ch1.individualStep);    // Bits 2-0 - Individual step
    	
        return result;
    } else if (address == 0xFF11) {
        // NR11 - Channel 1 Length Timer & Duty Cycle
        uint8_t result = 0;
    	
        result |= (ch1.waveDuty << 6); // Bits 7-6 - Wave duty
        result |= (0b111111     << 0); // Bits 5-0 - Length timer (write-only)
    	
        return result;
    } else if (address == 0xFF12) {
        // NR12 - Channel 1 Volume Envelope
        uint8_t result = 0;
    	
        result |= (ch1.initialVolume << 4);      // Bits 7-4 - Initial volume
        result |= (ch1.envDir << 3);             // Bit 3 - Envelope direction
        result |= (ch1.envelopePace & 0b00000111);  // Bits 2-0 - Sweep pace
    	
        return result;
    } else if (address == 0xFF13) {
        // NR13 - Channel 1 Period Low (Write-only)
        return 0xFF;
    } else if (address == 0xFF14) {
        // NR14 - Channel 1 Period High & Control
        uint8_t result = 0;
    	
        result |= (1                << 7); // Bit 7 - Trigger        - Write Only
        result |= (ch1.lengthEnable << 6); // Bit 6 - Length enable
        result |= (0b00111111       << 0); // Bits 2-0 - Period high - Write Only
    	
        return result;
    } else if(address == 0xFF15) {
	    // CH2 Doesn't have a sweep
    	// I suppose we are supposed to ignore this?
    	return 0xFF;
    } else if(address == 0xFF16) {
    	// NR21
    	uint8_t result = 0;
    	
    	result |= (ch2.waveDuty << 6); // Bits 7-6 - Wave duty
    	result |= (0b111111     << 0); // Bits 5-0 - Length timer (write-only)
    	
    	return result;
    } else if(address == 0xFF17) {
    	// NR22
    	uint8_t result = 0;
    	
    	result |= (ch2.initialVolume << 4);      // Bits 7-4 - Initial volume
    	result |= (ch2.envDir << 3);             // Bit 3    - Envelope direction
    	result |= (ch2.envelopePace & 0b00000111);  // Bits 2-0 - Sweep pace
    	
    	return result;
    } else if(address == 0xFF18) {
    	// NR23 Channel 2 Period Low (Write-only)
    	return 0xFF;
    } else if(address == 0xFF19) {
    	// NR24
    	uint8_t result = 0;
    	
    	result |= (1                << 7); // Bit 7 - Trigger        - Write Only
    	result |= (ch2.lengthEnable << 6); // Bit 6 - Length enable
    	result |= (0b00111111       << 0); // Bits 2-0 - Period high - Write Only
    	
    	return result;
    } else if (address == 0xFF1A) {
        // NR30 - Channel 3 DAC Enable
        uint8_t result = 0;
    	
        result |= (ch3.DAC   << 7); // Bit 7 - DAC Enable
    	result |= (0b1111111 << 0); // Bits 6-0 - Always set
    	
        return result;
    } else if (address == 0xFF1B) {
        // NR31 - Channel 3 Length Timer (Write-only)
        return 0xFF;
    } else if (address == 0xFF1C) {
        // NR32 - Channel 3 Output Level
        uint8_t result = 0;
    	
    	result |= (1               << 7); // Bit  7   - Always set 
        result |= (ch3.outputLevel << 5); // Bits 6-5 - Output level
		result |= (0b11111         << 0); // Bits 0-4 - Always set
    	
        return result;
    } else if (address == 0xFF1D) {
        // NR33 - Channel 3 Period Low (Write-only)
        return 0xFF;
    } else if (address == 0xFF1E) {
        // NR34 - Channel 3 Period High & Control
        uint8_t result = 0;
		
    	// 0b10111111 - 191
        result |= (1                << 7); // Bit 7    - Trigger
        result |= (ch3.lengthEnable << 6); // Bit 6    - Length enable
    	result |= (0b111            << 3); // Bits 3-6 - Always set      
        result |= (0b111            << 0); // Bits 2-0 - Period high
    	
        return result;
    } else if (address >= 0xFF30 && address <= 0xFF3F) {
        // Wave Pattern RAM
		if(ch3.DAC)
			return 0xFF;
		
        return ch3.waveform[address - 0xFF30];
    } else if (address == 0xFF20) {
        // NR41 - Channel 4 Length Timer (Write-only)
        return 0xFF;
    } else if (address == 0xFF21) {
        // NR42 - Channel 4 Volume Envelope
        uint8_t result = 0;
    	
        result |= (ch4.initialVolume << 4); // Bits 7-4 - Initial volume
        result |= (ch4.envDir        << 3); // Bit  3   - Envelope direction
        result |= (ch4.sweepPace     << 0); // Bits 2-0 - Sweep pace
    	
        return result;
    } else if (address == 0xFF22) {
        // NR43 - Channel 4 Frequency & Polynomial Counter
        // TODO; I'm going to cry
        return 0;
    } else if (address == 0xFF23) {
        // NR44 - Channel 4 Control - write-only(I think)
        uint8_t result = 0;
		
    	result |= (1 << 7);                // Bit  7    - Always set
    	result |= (ch4.lengthEnable << 6); // Bit  6    - Length enable
    	result |= (0b111111 << 0);         // Bits 5-0 - Always set
    	
    	return result;
    }
	
    return 0xFF;
}

void APU::write8(uint16_t address, uint8_t data) {
	/**
	 * According to: https://gbdev.io/pandocs/Audio_Registers.html#ff26--nr52-audio-master-control
	 * 
	 * Turning the APU off drains less power (around 16%),
	 * but clears all APU registers and makes them read-only until turned back on, except NR521.
	 */
	if (!enabled && address != 0xFF26 && (address < 0xFF30) ||
		(Cartridge::mode == Color && (address != 0xFF11 && address != 0xFF21 && 
					address != 0xFF31 && address != 0xFF41))) {
		//printf("Ignoring %x\n", address);
		return;
	}
	
	if(address == 0xFF26) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff26--nr52-audio-master-control
		
		bool wasEnabled = enabled;
		
		// Bit 7 - Audio on/off
		enabled = check_bit(data, 7);
		
		// Apparently this is ready-only:
		// CHn on? (Read-only): Each of these four bits allows checking whether channels are active.
		// Writing to those does not enable or disable the channels, despite many emulators behaving as if.
		
		// Bit 3 - CH4 on?
		//ch4.enabled = check_bit(data, 3);
		
		// Bit 2 - CH3 on?
		//ch3.enabled = check_bit(data, 2);
		
		// Bit 1 - CH2 on?
		//ch2.enabled = check_bit(data, 1);
		
		// Bit 0 - CH1 on?
		//ch1.enabled = check_bit(data, 0);

		if(!enabled)
			printf("its off?\n");
		
		// APU is powering off
		if(wasEnabled && !enabled) {
			printf("Powering off\n");

			ch1.reset();
			ch2.reset();
			ch3.reset();
			ch4.reset();
			
			vinLeft = false;
			vinRight = false;
			leftVolume = 0;
			rightVolume = 0;
			counter = 0;
		}
	} else if(address == 0xFF25) {
		// 	https://gbdev.io/pandocs/Audio_Registers.html#ff25--nr51-sound-panning
		
		// Bit 7 - CH4 Left
		ch4.left = check_bit(data, 7);
		
		// Bit 6 - CH3 Left
		ch3.left = check_bit(data, 6);
		
		// Bit 5 - CH2 Left
		ch2.left = check_bit(data, 5);
		
		// Bit 4 - CH1 Left
		ch1.left = check_bit(data, 4);
		
		// Bit 3 - CH4 Right
		ch4.right = check_bit(data, 3);
		
		// Bit 2 - CH3 Right
		ch3.right = check_bit(data, 2);
		
		// Bit 1 - CH2 Right
		ch2.right = check_bit(data, 1);
		
		// Bit 0 - CH4 Right
		ch1.right = check_bit(data, 0);
	} else if(address == 0xFF24) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff24--nr50-master-volume--vin-panning
		
		// Bit 7 - VIN Left
		vinLeft = check_bit(data, 7);
		
		// Bit 7, 5 and 4 - Left volume
		leftVolume = (data & 0b01110000) >> 4;
		
		// Bit 3
		vinRight = check_bit(data, 3);
		
		// Bit 2, 1 and 0 - Right volume
		rightVolume = data & 0b00000111;
	} else if(address == 0xFF10) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff10--nr10-channel-1-sweep
		
		/**
		 * Bit 6, 5 and 4 - Pace
		 * 
		 * TODO; Note that the value written to this field is not re-read by the hardware,
		 * until a sweep iteration completes, or the channel is (re)triggered.
		 * 
		 * However, if 0 is written to this field,
		 * then iterations are instantly disabled (but see below),
		 * and it will be reloaded as soon as it’s set to something else.
		 */
		ch1.sweepPace = (data & 0b01110000) >> 4;
		
		/**
		 * Bit 3 - Direction
		 *
		 * 0 - Addition (period increases)
		 * 1 - Subreaction (period decreases)
		 */
		ch1.swpDirection = check_bit(data, 3);
		
		// Bit 2, 1 and 0 - Individual step
		ch1.individualStep = (data & 0b00000111);
	} else if(address == 0xFF11) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff11--nr11-channel-1-length-timer--duty-cycle

		if(!enabled)
			data &= 0x3F;
		
		// Bit 7 and 6 - Wave duty
		ch1.waveDuty = (data & 0b11000000) >> 6;
		
		/**
		 * Bit 5 to 0 - Initial length timer
		 *	
		 * This is a write only!
		 * https://gbdev.io/pandocs/Audio.html#length-timer
		*/
		ch1.initialTimer = data & 0b00111111;
	} else if(address == 0xFF12) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff12--nr12-channel-1-volume--envelope
		
		/** 
		 * Bit 7 through 4 - Initial volume
		 * 
		 * R/W - Cannot be updated anywhere else!	
		 */
		ch1.initialVolume = (data & 0b11110000) >> 4;
		
		/**
		 * Bit 3 - Env dir
		 * 
		 * Envelope direction
		 * 0 = Decrease volume over time
		 * 1 = Increase volume over time
		 */
		ch1.envDir = check_bit(data, 3);
		
		// Bit 2, 1 and 0 - Sweep pace
		ch1.envelopePace = data & 0b00000111;
		
		/**
		 * TODO; Setting bits 3-7 to 0,
		 * turns off DAC and the channel as well!
		 */
	} else if(address == 0xFF13) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff13--nr13-channel-1-period-low-write-only
		
		ch1.periodLow = data;
	} else if(address == 0xFF14) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff14--nr14-channel-1-period-high--control
		
		/**
		 * Bit 7 - Trigger Write-only
		 *
		 * Writting any value to NR14(this address),
		 * with this bet being set, it triggers the channel.
		 */
		ch1.trigger = check_bit(data, 7);
		
		if(ch1.trigger)
			ch1.updateTrigger();

		// Bit 6 - Length enable - R/W
		ch1.lengthEnable = check_bit(data, 6);
		
		/**
		 * Bit 2, 1 and 0 - Period
		 *
		 * This is the upper 3 bits of the period value;
		 * the lower 8 bits are from NR13 (ch1.periodLow)
		 */
		ch1.periodHigh = data & 0b00000111;
	} else if(address == 0xFF15) {
		// https://gbdev.io/pandocs/Audio_Registers.html#sound-channel-2--pulse
		
		/*
		 * Channel 2 doesn't have sweep.
		 */
	} else if(address == 0xFF16) {
		// NR21, exact same behaviour as NR11 but for CH2.
		
		// https://gbdev.io/pandocs/Audio_Registers.html#ff11--nr11-channel-1-length-timer--duty-cycle

		if(!enabled)
			data &= 0x3F;
		
		// Bit 7 and 6 - Wave duty
		ch2.waveDuty = (data & 0b11000000) >> 6;
		
		/**
		 * Bit 5 to 0 - Initial length timer
		 *	
		 * This is a write only!
		 * https://gbdev.io/pandocs/Audio.html#length-timer
		*/
		ch2.initialTimer = data & 0b00111111;
	} else if(address == 0xFF17) {
		// NR22, exact same behaviour as NR12 but for CH2.
		
		// https://gbdev.io/pandocs/Audio_Registers.html#ff12--nr12-channel-1-volume--envelope
		
		/** 
		 * Bit 7 through 4 - Initial volume
		 * 
		 * R/W - Cannot be updated anywhere else!	
		 */
		ch2.initialVolume = (data & 0b11110000) >> 4;
		
		/**
		 * Bit 3 - Env dir
		 *
		 * Envelope direction
		 * 0 = Decrease volume over time
		 * 1 = Increase volume over time
		 */
		ch2.envDir = check_bit(data, 3);
		
		// Bit 2, 1 and 0 - Sweep pace
		ch2.envelopePace = data & 0b00000111;
		
		/**
		 * TODO; Setting bits 3-7 to 0,
		 * turns off DAC and the channel as well!
		 */
	} else if(address == 0xFF18) {
		// NR23, exact same behaviour as NR13 but for CH2.
		
		// https://gbdev.io/pandocs/Audio_Registers.html#ff13--nr13-channel-1-period-low-write-only
		
		ch2.periodLow = data;
	} else if(address == 0xFF19) {
		// NR24, exact same behaviour as NR14 but for CH2.
		
		// https://gbdev.io/pandocs/Audio_Registers.html#ff14--nr14-channel-1-period-high--control
		
		/**
		 * Bit 7 - Trigger Write-only
		 *
		 * Writting any value to NR14(this address),
		 * with this bet being set, it triggers the channel.
		 */
		ch2.trigger = check_bit(data, 7);

		if(ch2.trigger)
			ch2.updateTrigger();
		
		// Bit 6 - Length enable - R/W
		ch2.lengthEnable = check_bit(data, 6);
		
		/**
		 * Bit 2, 1 and 0 - Period
		 *
		 * This is the upper 3 bits of the period value;
		 * the lower 8 bits are from NR13 (ch1.periodLow)
		 */
		ch2.periodHigh = data & 0b00000111;
	} else if(address == 0xFF1A) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff1a--nr30-channel-3-dac-enable
		
		// Bit 7 - DAC
		ch3.DAC = check_bit(data, 7);
	} else if(address == 0xFF1B) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff1b--nr31-channel-3-length-timer-write-only
		
		// Bit 7 through 0 - Initial length timer
		ch3.initialTimer = data;
	} else if(address == 0xFF1C) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff1b--nr31-channel-3-length-timer-write-only
		
		// Bit 6 and 5 - Output level
		ch3.outputLevel = (data & 0b01100000) >> 5;
	} else if(address == 0xFF1D) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff1d--nr33-channel-3-period-low-write-only
		
		// This is write only - Period low
		ch3.periodLow = data;
	} else if(address == 0xFF1E) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff1e--nr34-channel-3-period-high--control
		
		// Bit 7 - Trigger
		ch3.trigger = check_bit(data, 7);

		if(ch3.trigger)
			ch3.updateTrigger();
		
		// Bit 6 - Length enable
		ch3.lengthEnable = check_bit(data, 6);
		
		// Bit 2, 1 and 0 - Period
		ch3.periodHigh = data & 0b00000111;
	} else if(address >= 0xFF30 && address <= 0xFF3F) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff30ff3f--wave-pattern-ram
		if(ch3.DAC)
			return;
		
		ch3.waveform[address - 0xFF30] = data;
	} else if(address == 0xFF20) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff20--nr41-channel-4-length-timer-write-only
		
		// Bit 5 through 0 - Initial timer
		ch4.initialTimer = data & 0b00111111;
	} else if(address == 0xFF21) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff21--nr42-channel-4-volume--envelope
		
		/** 
		 * Bit 7 through 4 - Initial volume
		 * 
		 * R/W - Cannot be updated anywhere else!	
		 */
		ch4.initialVolume = (data & 0b11110000) >> 4;
		
		/**
		 * Bit 3 - Env dir
		 *
		 * Envelope direction
		 * 0 = Decrease volume over time
		 * 1 = Increase volume over time
		 */
		ch4.envDir = check_bit(data, 3);
		
		// Bit 2, 1 and 0 - Sweep pace
		ch4.sweepPace = data & 0b00000111;
		
		/**
		 * TODO; Setting bits 3-7 to 0,
		 * turns off DAC and the channel as well!
		 */
	} else if(address == 0xFF22) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff22--nr43-channel-4-frequency--randomness
		
		// Bit 7 through 4 - Clock shift
		ch4.clockShift = (data & 0b11110000) >> 4;
		
		// Bit 3 - LFSR width
		ch4.lsfrWidth = check_bit(data, 3);
		
		// Bit 2, 1 and 0 - Clock divider
		ch4.clockDivider = data & 0b00000111;
	} else if(address == 0xFF23) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff23--nr44-channel-4-control
		
		// Bit 7 - Trigger W/R
		ch4.trigger = check_bit(data, 7);

		if(ch4.trigger)
			ch4.updateTrigger();
		
		// Bit 6 - Length enable - W/R
		ch4.lengthEnable = check_bit(data, 6);
	} else if(address == 0xFF1F) {
		// Unused
	} else {
		printf("Unknown APU address: %x\n", address);
		std::cerr << "";
	}
}

// https://www.libsdl.org/release/SDL-1.2.15/docs/html/guideaudioexamples.html
void APU::fill_audio(void* udata, Uint8* stream, int len) {
	APU* apu = static_cast<APU*>(udata);
	
	Uint8* out = (Uint8*)stream;
	//int16_t* out = (int16_t*)(stream);
	
	int length = len / sizeof(out[0]);
	
	apu->newSamples.resize(length / 2);
	
	// CPU clock rate
	auto ticks = 4 * 1024 * 1024;
	auto sampleRate = ticks / 44100;
	
	int x = 0;
	for(int i = 0; i < length; i += 2) {
		uint8_t ch1 = 0;
		uint8_t ch2 = 0;
		uint8_t ch3 = 0;
		uint8_t ch4 = 0;
		
		for(int j = 0; j <= sampleRate; j++) {
			//apu->tick(4);
			
			// TODO; Ensure those are mapped to [0, 1.0] instead
			ch1 = apu->ch1.sample(1);
			ch2 = apu->ch2.sample(1);
			ch3 = apu->ch3.sample(1);
			ch4 = apu->ch4.sample(1);
		}
		
		// Ik this is scuffy
		if(!apu->enableCh1) ch1 = 0;
		if(!apu->enableCh2) ch2 = 0;
		if(!apu->enableCh3) ch3 = 0;
		if(!apu->enableCh4) ch4 = 0;
		
		// TODO; What is vin left/right?
		
		if(!apu->enableAudio || !apu->enabled) {
			continue;
		}
		
 		out[i + 0] = 
			(((ch1 * apu->ch1.left )  + (ch2 * apu->ch2.left))) +
			(((ch3 * apu->ch3.left )  + (ch4 * apu->ch4.left)))
			+ (apu->leftVolume  + 1) / 4;
		
		out[i + 1] =
			(((ch1 * apu->ch1.right) + (ch2 * apu->ch2.right))) +
			(((ch3 * apu->ch3.right) + (ch4 * apu->ch4.right)))
			+ (apu->rightVolume + 1) / 4;
		
		apu->newSamples[x++] = out[i] * 10;
	}
}
