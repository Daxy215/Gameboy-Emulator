#include "Cartridge.h"

#include <iostream>

Mode Cartridge::mode = DMG;

void Cartridge::decode(const std::vector<uint8_t>& data) {
    // From 0x100 - 0x014F
    
    // Information provided from; https://gbdev.io/pandocs/The_Cartridge_Header.html
    for(size_t i = 0x0100; i <= 0x014F; i++) {
        if(i >= 0x0100 && i <= 0x0103) {
            // Entry point, just ignore
            
        }
        
        if(i >= 0x0104 && i <= 0x0133) {
            // Nintendo logo
            
            /**
             * Must match;
             * It must match the following (hexadecimal) dump,
             * otherwise the boot ROM won’t allow the game to run:
             *
             * CE ED 66 66 CC 0D 00 0B 03 73 00 83 00 0C 00 0D
             * 00 08 11 1F 88 89 00 0E DC CC 6E E6 DD DD D9 99
             * BB BB 67 63 6E 0E EC CC DD DC 99 9F BB B9 33 3E
             *
             * VISUAL AID;
             * https://github.com/ISSOtm/gb-bootroms/blob/2dce25910043ce2ad1d1d3691436f2c7aabbda00/src/dmg.asm#L259-L269
             */
            
            //printf("%x", data[i]);
        }
        
        if(i == 0x0134/* && i <= 0x0142*/) {
            /**
             * Title;
             *
             * These bytes contain the title of the game in upper case ASCII.
             * If the title is less than 16 characters long, the remaining bytes should be padded with $00s.
             * 
             * Parts of this area actually have a different meaning on later cartridges,
             * reducing the actual title size to 15 ($0134–$0142) or 11 ($0134–$013E) characters; see below.
             */
            
            for(size_t j = 0; j < 16; j++) {
                uint8_t val = data[j + i];
                
                if(val != 0)
                    title += val;
            }
            
            std::cerr << "Title; " << title << "\n";
        }
        
        if(i == 0x013F/* && i <= 0x0142*/) {
            /**
             * Manufacturer code;
             * 
             * In older cartridges these bytes were part of the Title (see above).
             * In newer cartridges they contain a 4-character manufacturer code (in uppercase ASCII).
             * The purpose of the manufacturer code is unknown.
             */
            
            for(size_t j = 0; j < 4; j++)
                manufacturer += data[i + j];
            
            std::cerr << "Manufacturer: " << manufacturer << "\n";
        }
        
        if(i == 0x0143) {
            /**
             * https://gbdev.io/pandocs/The_Cartridge_Header.html#0143--cgb-flag
             * CGB flag
             * 
             * 80 - Game supports CGB though can work with Game Boy
             * C0 - Only CGB
             * 
             * Igoring PGB Mode
             */
            if (data[i] >= 0x80) {
                if ((data[i] & 0xC0) == 0xC0) {
                    std::cerr << "Color only mode\n";
                    mode = Color;
                } else {
                    std::cerr << "Color/Normal mode\n";
                    mode = DMG;
                }
            } else {
                std::cerr << "Normal mode\n";
                mode = DMG;
            }
        }
        
        if(i >= 0x0144 && i <= 0x0145) {
            // New licensee code
            // It is only meaningful if the Old licensee is exactly $33.
            
            std::string newLicenseeCode = "";
            
            for(size_t j = i; j < 2; j++, j++) {
                newLicenseeCode += data[j];
            }
            
            //std::cout << "Soo; " << newLicenseeCode << "\n";
        }
        
        if(i == 0x0147) {
            // Cartridge type
            // https://gbdev.io/pandocs/The_Cartridge_Header.html#0147--cartridge-type
            
            type = getCartridgeType(data[i]);
            std::cerr << "Type; " << cartridgeTypeToString(type) << "\n";
        }
        
        if(i == 0x0148) {
            // ROM Size
            // https://gbdev.io/pandocs/The_Cartridge_Header.html#0148--rom-size
            
            romSize = static_cast<uint8_t>(32 * (1 << data[i]));
            
            switch (data[i]) {
                case 0: romBanks = 2; break; // No banking
                case 1: romBanks = 4; break;
                case 2: romBanks = 8; break;
                case 3: romBanks = 16; break;
                case 4: romBanks = 32; break;
                case 5: romBanks = 64; break;
                case 6: romBanks = 128; break;
                case 7: romBanks = 256; break;
                case 8: romBanks = 512; break;
                
                /**
                 * These values are most inaccurate..
                 * Most likely as they are unknown.
                 *
                 * According to: https://gbdev.io/pandocs/The_Cartridge_Header.html#weird_rom_sizes
                 */
                /*case 52: romBanks = 72; break;
                case 53: romBanks = 80; break;
                case 54: romBanks = 96; break;*/
                default: std::cerr << "Unknown ROM Size of: " << std::hex << std::to_string(data[i]) << "\n"; break;
            }
        }
        
        if(i == 0x0149) {
            // RAM Size
            // https://gbdev.io/pandocs/The_Cartridge_Header.html#0149--ram-size
            
            // TODO; If data[i] == 0 - Unused aka NO RAM.
            
            ramSize = 0;
            
            switch (data[i]) {
                case 0: ramBanks = 0;  ramSize = 0; break; // No RAM
                case 1: ramBanks = 0;                  break; // Unused
                case 2: ramBanks = 1;  ramSize = 8;    break;
                case 3: ramBanks = 4;  ramSize = 32;   break;
                case 4: ramBanks = 16; ramBanks = 128; break;
                case 5: ramBanks = 8;  ramSize = 64;   break;
                default: std::cerr << "Unknown RAM Size of: " << std::hex << std::to_string(data[i]) << "\n"; break;
            }
        }
    }
}

enums::CartridgeType Cartridge::getCartridgeType(uint8_t data) {
    // too lazy :}
    using namespace enums;
    
    switch (data) {
        case ROM_ONLY: return ROM_ONLY;
        case MBC1: return MBC1;
        case MBC1_RAM: return MBC1_RAM;
        case MBC1_RAM_BATTERY: return MBC1_RAM_BATTERY;
        case MBC2: return MBC2;
        case MBC2_BATTERY: return MBC2_BATTERY;
        case ROM_RAM: return ROM_RAM;
        case ROM_RAM_BATTERY: return ROM_RAM_BATTERY;
        case MMM01: return MMM01;
        case MMM01_RAM: return MMM01_RAM;
        case MMM01_RAM_BATTERY: return MMM01_RAM_BATTERY;
        case MBC3_TIMER_BATTERY: return MBC3_TIMER_BATTERY;
        case MBC3_TIMER_RAM_BATTERY: return MBC3_TIMER_RAM_BATTERY;
        case MBC3: return MBC3;
        case MBC3_RAM: return MBC3_RAM;
        case MBC3_RAM_BATTERY: return MBC3_RAM_BATTERY;
        case MBC5: return MBC5;
        case MBC5_RAM: return MBC5_RAM;
        case MBC5_RAM_BATTERY: return MBC5_RAM_BATTERY;
        case MBC5_RUMBLE: return MBC5_RUMBLE;
        case MBC5_RUMBLE_RAM: return MBC5_RUMBLE_RAM;
        case MBC5_RUMBLE_RAM_BATTERY: return MBC5_RUMBLE_RAM_BATTERY;
        case MBC6: return MBC6;
        case MBC7_SENSOR_RUMBLE_RAM_BATTERY: return MBC7_SENSOR_RUMBLE_RAM_BATTERY;
        case POCKET_CAMERA: return POCKET_CAMERA;
        case BANDAI_TAMA5: return BANDAI_TAMA5;
        case HUC3: return HUC3;
        case HUC1_RAM_BATTERY: return HUC1_RAM_BATTERY;
        
        default: return UNKNOWN_CARTRIDGE;  // Handle unrecognized cartridge types
    }
}

const char* Cartridge::cartridgeTypeToString(enums::CartridgeType type) {
    switch (type) {
    case enums::ROM_ONLY: return "ROM Only";
    case enums::MBC1: return "MBC1";
    case enums::MBC1_RAM: return "MBC1 + RAM";
    case enums::MBC1_RAM_BATTERY: return "MBC1 + RAM + Battery";
    case enums::MBC2: return "MBC2";
    case enums::MBC2_BATTERY: return "MBC2 + Battery";
    case enums::ROM_RAM: return "ROM + RAM";
    case enums::ROM_RAM_BATTERY: return "ROM + RAM + Battery";
    case enums::MMM01: return "MMM01";
    case enums::MMM01_RAM: return "MMM01 + RAM";
    case enums::MMM01_RAM_BATTERY: return "MMM01 + RAM + Battery";
    case enums::MBC3_TIMER_BATTERY: return "MBC3 + Timer + Battery";
    case enums::MBC3_TIMER_RAM_BATTERY: return "MBC3 + Timer + RAM + Battery";
    case enums::MBC3: return "MBC3";
    case enums::MBC3_RAM: return "MBC3 + RAM";
    case enums::MBC3_RAM_BATTERY: return "MBC3 + RAM + Battery";
    case enums::MBC5: return "MBC5";
    case enums::MBC5_RAM: return "MBC5 + RAM";
    case enums::MBC5_RAM_BATTERY: return "MBC5 + RAM + Battery";
    case enums::MBC5_RUMBLE: return "MBC5 + Rumble";
    case enums::MBC5_RUMBLE_RAM: return "MBC5 + Rumble + RAM";
    case enums::MBC5_RUMBLE_RAM_BATTERY: return "MBC5 + Rumble + RAM + Battery";
    case enums::MBC6: return "MBC6";
    case enums::MBC7_SENSOR_RUMBLE_RAM_BATTERY: return "MBC7 + Sensor + Rumble + RAM + Battery";
    case enums::POCKET_CAMERA: return "Pocket Camera";
    case enums::BANDAI_TAMA5: return "Bandai Tama5";
    case enums::HUC3: return "Huc3";
    case enums::HUC1_RAM_BATTERY: return "Huc1 + RAM + Battery";
    //case UNKNOWN_CARTRIDGE: return "Unknown Cartridge";
    default: return "Invalid Cartridge Type";
    }
}
