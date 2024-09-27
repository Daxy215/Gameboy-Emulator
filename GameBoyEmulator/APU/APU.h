#pragma once
#include "Channels/NoiseChanel.h"
#include "Channels/PulseChannel.h"
#include "Channels/WaveChannel.h"

// https://gbdev.io/pandocs/Audio.html

/**
 * According to; https://gbdev.io/pandocs/Audio.html#architecture
 *
 * There are 4 channels with three different types;
 *
 * First and second are "pulse" channels.
 * Which produces pulse width modulated waves,
 * that has four fixed pulse width settings.
 * 
 * The Third channel is a "wave" which produces,
 * arbitrary user-supplied waves.
 *
 * The fourth and final channel is a "noise" channel,
 * that produces a pseudo-random wave.
 */

/**
 * Just wanted to set up the structure,
 * won't work on this for a while
 */

class APU {
public:
	APU();
	void tick(uint32_t cycles);
	
	void beginPlaying();
	void playSound();
	
	uint8_t fetch8(uint16_t address);
	void write8(uint16_t address, uint8_t data);
	
	// Used to convert [-1, 1] to [-32768, 32767] for PCM
	short floatToPCM(float sample) {
		return static_cast<short>(sample * 32767.0f);
	}

private:
	uint32_t ticks = 0;
	
private:
	bool enabled = false;
	
	bool vinLeft = false, vinRight = false;
	
	uint8_t leftVolume = 0;
	uint8_t rightVolume = 0;
	
	PulseChannel ch1, ch2;
	WaveChannel ch3;
	NoiseChanel ch4;
	
	/**
	* 0 - Left
	* 1 - Right
	*/
	std::vector<std::vector<float>> outputChannels;
	
	uint8_t wavePattern[16];
};
