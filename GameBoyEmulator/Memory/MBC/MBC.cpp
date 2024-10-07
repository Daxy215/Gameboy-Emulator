#include <filesystem>
#include <fstream>
#include <iostream>
#include <filesystem>

#include "MBCS/MBC0/MBC0.h"
#include "MBCS/MBC1/MBC1.h"
#include "MBCS/MBC3/MBC3.h"
#include "MBCS/MBC5/MBD5.h"

MBC::MBC() {
	
}

MBC::MBC(Cartridge cartridge, std::vector<uint8_t> rom) : data(rom) {
	this->title = cartridge.title;
	
	switch (cartridge.type) {
	case enums::ROM_ONLY:
		curMBC = std::make_unique<MBC0>(cartridge, rom);
		
		break;
	case enums::MBC1:
		curMBC = std::make_unique<MBC1>(cartridge, rom);
		
		break;
	case enums::MBC1_RAM:
		curMBC = std::make_unique<MBC1>(cartridge, rom);
		
		break;
	case enums::MBC1_RAM_BATTERY:
		curMBC = std::make_unique<MBC1>(cartridge, rom);
		
		break;
	case enums::MBC3:
		curMBC = std::make_unique<MBC3>(cartridge, rom);
		
		break;
	case enums::MBC3_RAM_BATTERY:
		curMBC = std::make_unique<MBC3>(cartridge, rom);
		
		break;
	case enums::MBC3_TIMER_BATTERY:
		curMBC = std::make_unique<MBC3>(cartridge, rom);
		
		break;
	case enums::MBC3_TIMER_RAM_BATTERY:
		curMBC = std::make_unique<MBC3>(cartridge, rom);
		
		break;
	case enums::MBC5:
	case enums::MBC5_RAM:
	case enums::MBC5_RAM_BATTERY:
		curMBC = std::make_unique<MBC5>(cartridge, rom);
		
		break;
	}
}

void MBC::tick(uint16_t cycles) {
	/*ticks += cycles;
	
	if(ticks > 419430) {
		ticks = 0;
		
		if(curMBC->ramUpdatd) {
			curMBC->ramUpdatd = false;
			
		}
	}*/
}

uint8_t MBC::read(uint16_t address) {
	return curMBC->fetch8(address);
}

void MBC::write(uint16_t address, uint8_t data) {
	/*if(address == 41221) {
		std::cerr << "Saving?\n";
	}*/
	
	curMBC->write8(address, data);
}

uint8_t MBC::fetch8(uint16_t address) {
	return 0;
}

void MBC::write8(uint16_t address, uint8_t data) {
	
}

void MBC::load(const std::string& path) {
	std::ifstream stream(path, std::ios::binary);
	
	if(!stream) {
		std::cerr << "Error couldn't find save file to load! " << path << "\n";
		return;
	}
	
	stream.read(reinterpret_cast<char*>(curMBC->eram.data()), curMBC->eram.size());
	stream.close();
	
	std::cerr << "Loading from" << path << " was successful(I think)\n";
}

void MBC::save(const std::string& path) {
	std::ofstream stream(path, std::ios::binary);
	
	if(!stream) {
		std::cerr << "Creating a new save file at: " << path << "\n";
		
		std::filesystem::path savePath(path);
		std::filesystem::create_directories(savePath.parent_path());
		
		return;
	}
	
	stream.write(reinterpret_cast<const char*>(curMBC->eram.data()), curMBC->eram.size());
	stream.close();
	
	std::cerr << "Saving to " << path << " was successful(I think)\n";
}
