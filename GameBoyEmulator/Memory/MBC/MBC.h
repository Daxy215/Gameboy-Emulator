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
	uint8_t romSize = 0;
	uint8_t ramSize = 0;
	
	std::vector<uint8_t> rom;
	std::vector<uint8_t> eram;

private:
	/**
	 * Current active MBC.
	 * 
	 * This changes depending on the,
	 * cartridge type of the ROM.
	 */
	std::unique_ptr<MBC> curMBC;
};
