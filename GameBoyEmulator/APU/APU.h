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
	void tick();
	
	uint8_t fetch8(uint16_t address);
	void write8(uint16_t address, uint8_t data);
	
private:
	PulseChannel ch1, ch2;
	WaveChannel ch3;
	NoiseChanel c4;
};
