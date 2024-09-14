#include "Fetcher.h"

#include "../Memory/MMU.h"

void Fetcher::begin(uint16_t mapAddress, uint8_t tileLine) {
	curTileIndex = 0;
	curTileID = 0;
	this->mapAddress = mapAddress;
	this->tileLine = tileLine;
	
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
			
			curState = FetchTileData0;
			break;
		}
		
		case FetchTileData0: {
			uint16_t offset = 0x8000 + (static_cast<uint16_t>(curTileID) * 16);
			uint16_t address = offset + (static_cast<uint16_t>(tileLine) * 2);
			uint8_t data = mmu.fetch8(address);
			
			for(uint8_t i = 0; i < 8; i++) {
				pixels[i] = (data >> i) & 1;
			}
			
			curState = FetchTileData1;
			break;
		}
		
		case FetchTileData1: {
			uint16_t offset = 0x8000 + (static_cast<uint16_t>(curTileID) * 16);
			uint16_t address = offset + (static_cast<uint16_t>(tileLine) * 2) + 1;
			uint8_t data = mmu.fetch8(address);
			
			for(uint8_t i = 0; i < 8; i++) {
				pixels[i] |= (((data >> i) & 1) << 1);
			}
			
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
