#include "ExternalRAM.h"

ExternalRAM::ExternalRAM() {
	
}

/*
ExternalRAM::ExternalRAM(size_t size, bool hasBatteryBackup) {
	bankSize = 0; // 0x2000
}
*/

uint8_t ExternalRAM::read(uint16_t address) {
	return RAM[address/* * (currentBank * bankSize)*/];
}

void ExternalRAM::write(uint16_t address, uint8_t data) {
	RAM[address/* * (currentBank * bankSize)*/] = data;
}


