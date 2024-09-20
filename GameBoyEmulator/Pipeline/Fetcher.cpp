#include "Fetcher.h"

#include "../Memory/MMU.h"

void Fetcher::begin(uint16_t baseAddress, uint16_t mapAddress, uint8_t tileLine) {
	this->baseAddress = baseAddress;
	this->mapAddress = mapAddress;
	this->tileLine = tileLine;
	
	curTileIndex = 0;
	curTileID = 0;
	curState = FetchTileID;
	
	fifo.clear();
}

void Fetcher::tick() {
	this->ticks++;
	
	if(ticks < 2) return;
	ticks = 0;
	
	switch (curState) {
		case FetchTileID: {
			curTileID = mmu.fetch8(mapAddress + curTileIndex);
			
			// https://gbdev.io/pandocs/Tile_Maps.html#bg-map-attributes-cgb-mode-only
			// uint8_t flags = mmu.fetch8(0x2000 + mapAddress + curTileIndex);
			
			curState = FetchTileData0;
			break;
		}
		
		case FetchTileData0: {
			fetchData(0);
			
			curState = FetchTileData1;
			break;
		}
		
		case FetchTileData1: {
			fetchData(1);
			
			curState = PushToFIFO;
			break;
		}
		
		case PushToFIFO: {
			if (fifo.size() <= 8) {
				for(int i = 7; i >= 0; i--) {
					fifo.push(pixels[i]);
				}
			}
			
			curTileIndex++;
			
			curState = FetchTileID;
			break;
		}
	}
	
	this->ticks--;
}

void Fetcher::fetchData(uint8_t index) {
	uint16_t offset = 0;
	
	if(baseAddress == 0x8000) {
		offset = baseAddress + (static_cast<uint16_t>(curTileID)) * 16;
	} else {
		offset = baseAddress + static_cast<uint16_t>(static_cast<int16_t>(static_cast<int8_t>(curTileID)) + 128) * 16;
	}
	
	uint16_t address = offset + (static_cast<uint16_t>(tileLine) * 2) + index;
	uint8_t data = mmu.fetch8(address);
	
	for(uint8_t i = 0; i < 8; i++) {
		if(index == 0)
			pixels[i] = (data >> i) & 1;
		else
			pixels[i] |= (((data >> i) & 1) << 1);
	}
}
