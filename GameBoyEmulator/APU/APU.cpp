#include "APU.h"

#include "../Utility/Bitwise.h"

#include <windows.h>
#include <mmsystem.h>
#include <vector>
#include <iostream>
#include <thread>

#pragma comment(lib, "winmm.lib")

APU::APU() {
	outputChannels.resize(2, std::vector<float>(44110));
}

void APU::tick(uint32_t cycles) {
	// APU Disabled
	if (!enabled || (!ch1.enabled && !ch2.enabled && !ch3.enabled && !ch4.enabled)) {
		return;
	}
	
	ticks++;
	
	// Generate samples from each channel
	uint8_t sampleCh1 = ch1.sample(cycles);
	uint8_t sampleCh2 = ch2.sample(cycles);
	uint8_t sampleCh3 = ch3.sample(cycles);
	uint8_t sampleCh4 = ch4.sample(cycles);
	
	// Mix the channels
	float mixedSample = sampleCh1 + sampleCh2 + sampleCh3 + sampleCh4;
	
	// Apply the left and right volume settings
	float leftSample = mixedSample * static_cast<float>(leftVolume) / 7;  // Scale by left volume (0-7)
	float rightSample = mixedSample * static_cast<float>(rightVolume) / 7; // Scale by right volume (0-7)
	
	// Apply optional VIN settings (if connected to external audio sources)
	if (vinLeft) {
		leftSample += 4;
	}
	
	if (vinRight) {
		rightSample += 4;
	}
	
	outputChannels[0][ticks] = leftSample;
	outputChannels[1][ticks] = rightSample;
	
	/**
	 * 44100 samples / sec * 0.05 = 2205.
	 *
	 * So imma use this for now..
	 */
	if(ticks >= 44100) {
		ticks = 0;
		
		beginPlaying();
	}
}

void APU::beginPlaying() {
	std::thread playbackThread(&APU::playSound, this);
	
	playbackThread.detach();
}

void APU::playSound() {
	HWAVEOUT hWaveOut = {};
	WAVEFORMATEX wfx = {0};
    
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 2;  // 2 for Stereo
	wfx.nSamplesPerSec = 44100;
	wfx.wBitsPerSample = 16;
	wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    
	// Open wave out device
	//if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, DWORD_PTR(waveCallback), (DWORD_PTR)this, CALLBACK_FUNCTION | WAVE_ALLOWSYNC) != MMSYSERR_NOERROR) {
	if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
		std::cerr << "Failed to open wave out device!" << '\n';
		return;
	}
    
	// Prepare audio samples
	std::vector<short> pcmSamples;
	
	size_t numSamples = outputChannels[0].size();
	for (size_t i = 0; i < numSamples; i++) {
		pcmSamples.push_back(floatToPCM(outputChannels[0][i])); // Left channel
		pcmSamples.push_back(floatToPCM(outputChannels[1][i])); // Right channel
	}
    
	// Create and prepare a WAVEHDR structure
	WAVEHDR waveHeader = {0};
	
	waveHeader.lpData = (LPSTR)pcmSamples.data();
	//waveHeader.lpData = (LPSTR)outputChannels.data();
	waveHeader.dwBufferLength = pcmSamples.size() * sizeof(short);
	waveHeader.dwFlags = 0;
	waveHeader.dwLoops = 0;
	
	waveOutPrepareHeader(hWaveOut, &waveHeader, sizeof(WAVEHDR));
    
	// Send the block of audio data to the audio device
	if (waveOutWrite(hWaveOut, &waveHeader, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
		std::cerr << "Failed to play audio!" << '\n';
	}
    
	// Wait until the block has finished playing
	while (waveOutUnprepareHeader(hWaveOut, &waveHeader, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING) {
		Sleep(100);
	}
	
	// Cleanup
	waveOutClose(hWaveOut);
}

uint8_t APU::fetch8(uint16_t address) {
    if (address == 0xFF26) {
        // NR52 - Audio Master Control
        uint8_t result = 0;
    	
        result |= (enabled << 7);      // Bit 7 - Audio on/off
        result |= (ch4.enabled << 3);  // Bit 3 - Channel 4 status
        result |= (ch3.enabled << 2);  // Bit 2 - Channel 3 status
        result |= (ch2.enabled << 1);  // Bit 1 - Channel 2 status
        result |= (ch1.enabled << 0);  // Bit 0 - Channel 1 status
    	
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
    	
        result |= (ch1.pace << 4);       // Bits 6-4 - Sweep pace
        result |= (ch1.direction << 3);  // Bit 3 - Sweep direction
        result |= (ch1.individualStep);  // Bits 2-0 - Individual step
    	
        return result;
    } else if (address == 0xFF11) {
        // NR11 - Channel 1 Length Timer & Duty Cycle
        uint8_t result = 0;
    	
        result |= (ch1.waveDuty << 6);             // Bits 7-6 - Wave duty
        result |= (ch1.lengthTimer & 0b00111111);  // Bits 5-0 - Length timer (write-only)
    	
        return result;
    } else if (address == 0xFF12) {
        // NR12 - Channel 1 Volume Envelope
        uint8_t result = 0;
    	
        result |= (ch1.initialVolume << 4);      // Bits 7-4 - Initial volume
        result |= (ch1.envDir << 3);             // Bit 3 - Envelope direction
        result |= (ch1.sweepPace & 0b00000111);  // Bits 2-0 - Sweep pace
    	
        return result;
    } else if (address == 0xFF13) {
        // NR13 - Channel 1 Period Low (Write-only)
        return 0xFF;
    } else if (address == 0xFF14) {
        // NR14 - Channel 1 Period High & Control
        uint8_t result = 0;
    	
        result |= (ch1.trigger << 7);            // Bit 7 - Trigger
        result |= (ch1.lengthEnable << 6);        // Bit 6 - Length enable
        result |= (ch1.periodHigh & 0b00000111);  // Bits 2-0 - Period high
    	
        return result;
    } else if(address == 0xFF15) {
	    // CH2 Doesn't have a sweep
    	// I suppose we are supposed to ignore this?
    	return 0xFF;
    } else if(address == 0xFF16) {
    	// NR1
    	uint8_t result = 0;
    	
    	result |= (ch2.waveDuty << 6);             // Bits 7-6 - Wave duty
    	result |= (ch2.lengthTimer & 0b00111111);  // Bits 5-0 - Length timer (write-only)
    	
    	return result;
    } else if(address == 0xFF17) {
    	// NR22
    	uint8_t result = 0;
    	
    	result |= (ch2.initialVolume << 4);      // Bits 7-4 - Initial volume
    	result |= (ch2.envDir << 3);             // Bit 3    - Envelope direction
    	result |= (ch2.sweepPace & 0b00000111);  // Bits 2-0 - Sweep pace
    	
    	return result;
    } else if(address == 0xFF18) {
    	// NR23 Channel 2 Period Low (Write-only)
    	return 0xFF;
    } else if(address == 0xFF19) {
    	// NR24
    	uint8_t result = 0;
    	
    	result |= (ch2.trigger << 7);             // Bit 7    - Trigger
    	result |= (ch2.lengthEnable << 6);        // Bit 6    - Length enable
    	result |= (ch2.periodHigh & 0b00000111);  // Bits 2-0 - Period high
    	
    	return result;
    } else if (address == 0xFF1A) {
        // NR30 - Channel 3 DAC Enable
        uint8_t result = 0;
    	
        result |= (ch3.DAC << 7);  // Bit 7 - DAC Enable
    	
        return result;
    } else if (address == 0xFF1B) {
        // NR31 - Channel 3 Length Timer (Write-only)
        return 0xFF;
    } else if (address == 0xFF1C) {
        // NR32 - Channel 3 Output Level
        uint8_t result = 0;
    	
        result |= (ch3.outputLevel << 5);  // Bits 6-5 - Output level
    	
        return result;
    } else if (address == 0xFF1D) {
        // NR33 - Channel 3 Period Low (Write-only)
        return 0xFF;
    } else if (address == 0xFF1E) {
        // NR34 - Channel 3 Period High & Control
        uint8_t result = 0;
    	
        result |= (ch3.trigger << 7);             // Bit 7    - Trigger
        result |= (ch3.lengthEnable << 6);        // Bit 6    - Length enable
        result |= (ch3.periodHigh & 0b00000111);  // Bits 2-0 - Period high
    	
        return result;
    } else if (address >= 0xFF30 && address <= 0xFF3F) {
        // Wave Pattern RAM
        return wavePattern[address - 0xFF30];
    } else if (address == 0xFF20) {
        // NR41 - Channel 4 Length Timer (Write-only)
        return 0xFF;
    } else if (address == 0xFF21) {
        // NR42 - Channel 4 Volume Envelope
        uint8_t result = 0;
    	
        result |= (ch4.initialVolume << 4);      // Bits 7-4 - Initial volume
        result |= (ch4.envDir << 3);             // Bit 3    - Envelope direction
        result |= (ch4.sweepPace & 0b00000111);  // Bits 2-0 - Sweep pace
    	
        return result;
    } else if (address == 0xFF22) {
        // NR43 - Channel 4 Frequency & Polynomial Counter
        // TODO; I'm going to cry
        return 0xFF;
    } else if (address == 0xFF23) {
        // NR44 - Channel 4 Control - write-only(I think)
        return 0xFF;
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
	/*if(!enabled && address != 0xFF26) {
		return;
	}*/
	
	if(address == 0xFF26) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff26--nr52-audio-master-control
		
		// Bit 7 - Audio on/off
		enabled = check_bit(data, 7);
		
		// Bit 3 - CH4 on?
		ch4.enabled = check_bit(data, 4);
		
		// Bit 2 - CH3 on?
		ch3.enabled = check_bit(data, 4);
		
		// Bit 1 - CH2 on?
		ch2.enabled = check_bit(data, 4);
		
		// Bit 0 - CH1 on?
		ch1.enabled = check_bit(data, 4);
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
		ch1.pace = (data & 0b01110000) >> 4;
		
		/**
		 * Bit 3 - Direction
		 *
		 * 0 - Addition (period increases)
		 * 1 - Subreaction (period decreases)
		 */
		ch1.direction = check_bit(data, 3);
		
		// Bit 2, 1 and 0 - Individual step
		ch1.individualStep = (data & 0b00000111);
	} else if(address == 0xFF11) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff11--nr11-channel-1-length-timer--duty-cycle
		
		// Bit 7 and 6 - Wave duty
		ch1.waveDuty = (data & 0b11000000) >> 6;
		
		/**
		 * Bit 5 to 0 - Initial length timer
		 *	
		 * This is a write only!
		 * https://gbdev.io/pandocs/Audio.html#length-timer
		*/
		ch1.lengthTimer = data & 0b00111111;
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
		ch1.sweepPace = data & 0b00000111;
		
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
		 * So I suppose ignore this?
		 *
		 * TODO; Check this
		 */
	} else if(address == 0xFF16) {
		// NR21, exact same behavior as NR11 but for CH2.
		
		// https://gbdev.io/pandocs/Audio_Registers.html#ff11--nr11-channel-1-length-timer--duty-cycle
		
		// Bit 7 and 6 - Wave duty
		ch2.waveDuty = (data & 0b11000000) >> 6;
		
		/**
		 * Bit 5 to 0 - Initial length timer
		 *	
		 * This is a write only!
		 * https://gbdev.io/pandocs/Audio.html#length-timer
		*/
		ch2.lengthTimer = data & 0b00111111;
	} else if(address == 0xFF17) {
		// NR22, exact same behavior as NR12 but for CH2.
		
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
		ch2.sweepPace = data & 0b00000111;
		
		/**
		 * TODO; Setting bits 3-7 to 0,
		 * turns off DAC and the channel as well!
		 */
	} else if(address == 0xFF18) {
		// NR23, exact same behavior as NR13 but for CH2.
		
		// https://gbdev.io/pandocs/Audio_Registers.html#ff13--nr13-channel-1-period-low-write-only
		
		ch2.periodLow = data;
	} else if(address == 0xFF19) {
		// NR24, exact same behavior as NR14 but for CH2.
		
		// https://gbdev.io/pandocs/Audio_Registers.html#ff14--nr14-channel-1-period-high--control
		
		/**
		 * Bit 7 - Trigger Write-only
		 *
		 * Writting any value to NR14(this address),
		 * with this bet being set, it triggers the channel.
		 */
		ch2.trigger = check_bit(data, 7);
		
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
		ch3.lengthTimer = data;
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
		
		// Bit 6 - Length enable
		ch3.lengthEnable = check_bit(data, 6);
		
		// Bit 2, 1 and 0 - Period
		ch3.periodHigh = data & 0b00000111;
	} else if(address >= 0xFF30 && address <= 0xFF3F) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff30ff3f--wave-pattern-ram
		wavePattern[address - 0xFF30] = data;
	} else if(address == 0xFF20) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff20--nr41-channel-4-length-timer-write-only
		
		// Bit 5 through 0 - Initial timer
		ch4.lengthTimer = data & 0b00111111;
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
		ch4. clockDivider = data & 0b00000111;
	} else if(address == 0xFF23) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff23--nr44-channel-4-control
		
		// Bit 7 - Trigger W/R
		ch4.trigger = check_bit(data, 7);
		
		// Bit 6 - Length enable - W/R
		ch4.lengthEnable = check_bit(data, 6);
	}
	
	else {
		printf("Unknown APU address: %x\n", address);
		std::cerr << "";
	}
}
