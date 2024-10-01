#pragma once
#include <cstdint>
#include <memory>

#include "../Cartridge.h"

enum CartridgeType : int;

class MBC {
public:
	MBC();
	MBC(Cartridge cartridge, std::vector<uint8_t> rom);
	
	virtual ~MBC() = default;
	
	uint8_t read(uint16_t address);
	void write(uint16_t address, uint8_t data);
	
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
	std::vector<uint8_t> data;
	
	/**
	 * Current active MBC.
	 * 
	 * This changes depending on the,
	 * cartridge type of the ROM.
	 */
	std::unique_ptr<MBC> curMBC;
};
