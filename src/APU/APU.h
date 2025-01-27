#pragma once

#include <deque>
#include <queue>
#include <SDL.h>

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

	void init();
	void tick(uint32_t cycles);
	
	uint8_t fetch8(uint16_t address);
	void write8(uint16_t address, uint8_t data);
	
	// https://www.libsdl.org/release/SDL-1.2.15/docs/html/guideaudioexamples.html
	static void fill_audio(void *udata, Uint8 *stream, int len);
	
private:
	
	uint32_t ticks = 0;
	uint8_t counter = 0;
	
public:
	bool enabled = false;
	bool enableAudio = false;
	
	bool vinLeft = false, vinRight = false;
	
	uint8_t leftVolume = 0;
	uint8_t rightVolume = 0;
	
	// TODO; Why tf r these structs ;-;
	PulseChannel ch1, ch2;
	WaveChannel ch3;
	NoiseChanel ch4;
	
	//uint8_t wavePattern[16];
	
public:
	// TEST
	int sampleCounter = 0;
	
	bool enableCh1 = true;
	bool enableCh2 = true;
	bool enableCh3 = true;
	bool enableCh4 = true;
	
	std::queue<uint8_t> samples;
	std::vector<uint8_t> newSamples;
	
private:
	//const int bufferSize = 1024;
	/*float audioBuffer[BUFFER_SIZE];
	int audioBufferIndex = 0;
	*/
	
	/*SDL_AudioSpec audioSpec;
	SDL_AudioDeviceID audioDevice;*/

public:
	/*static uint8_t* audio_chunk;
	static uint32_t audio_len;
	static uint8_t* audio_pos;*/
};
