#pragma once

#include "FIFO.h"

class MMU;

enum FetcherState  {
	FetchTileID,
	FetchTileData0,
	FetchTileData1,
	PushToFIFO,
};

class Fetcher {
public:
	Fetcher(MMU& mmu)
		: curTileID(0), curTileIndex(0), tileLine(0), mapAddress(0), ticks(0), curState(), mmu(mmu) {
	}

	Fetcher(const Fetcher& other)
		: curTileID(0), curTileIndex(0), tileLine(0), mapAddress(0), ticks(0), curState(), mmu(other.mmu) {
	}

	void begin(uint16_t tileAddress, uint8_t tileLine);
	void tick();
	
private:
	uint8_t curTileID, curTileIndex;
	
	uint16_t tileLine;
	uint16_t mapAddress;
	
	uint16_t ticks;
	
	uint8_t pixels[8] = { 0 };

public:
	FIFO fifo;
	
private:
	FetcherState curState;

	MMU& mmu;
};
