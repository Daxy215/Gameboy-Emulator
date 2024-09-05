#pragma once
#include <cstdint>
#include <string>
#include <vector>

class ExternalRAM {
public:
	ExternalRAM();
	//ExternalRAM(size_t size, bool hasBatteryBackup = false);
	//~ExternalRAM();
	
	uint8_t read(uint16_t address);
	void write(uint16_t address, uint8_t data);
	
	void saveRAM(const std::string &fileName);
	void loadRAM(const std::string &fileName);
	
	void setBank(uint8_t bank);
    
private:
	uint8_t RAM[0x2000] = { 0 };  // The actual RAM storage
	uint8_t currentBank;       // Current bank selected
	bool hasBatteryBackup;     // Does the RAM have battery backup for saving?
	size_t bankSize;           // Size of each bank
	size_t totalSize;          // Total size of the RAM
};
