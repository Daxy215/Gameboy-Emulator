#pragma once

#include <cstdint>
#include <memory>

#include "../Cartridge.h"

class MBC {
public:
	MBC();
	MBC(Cartridge cartridge, std::vector<uint8_t> rom);
	
	virtual ~MBC() = default;
	
	uint8_t read(uint16_t address);
	void write(uint16_t address, uint8_t data);
	
public:
	void load(const std::string& path);
	void save(const std::string& path);
	
protected:
	virtual uint8_t fetch8(uint16_t address);
	virtual void write8(uint16_t address, uint8_t data);
	
protected:
	uint16_t romBanks = 0;
	
	// Technically, this CAN be a uint8
	uint16_t ramBanks = 0;
	
	std::vector<uint8_t> rom;
	std::vector<uint8_t> eram;
	
private:
	// Used for saving.. Ik it's scuffed
	std::string title;
	
	std::vector<uint8_t> data;
	
	/**
	 * Current active MBC.
	 * 
	 * This changes depending on the,
	 * cartridge type of the ROM.
	 */
	std::unique_ptr<MBC> curMBC;
};
