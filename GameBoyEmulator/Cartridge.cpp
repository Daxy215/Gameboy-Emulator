#include "Cartridge.h"

#include <iostream>

void Cartridge::decode(const std::vector<uint8_t>& data) {
    // From 0x100 - 0x014F
    
    // Information provided from; https://gbdev.io/pandocs/The_Cartridge_Header.html
    for(size_t i = 0x0100; i <= 0x014F; i++) {
        if(i >= 0x0100 && i <= 0x0103) {
            // Entry point, just ignore
            
        } else if(i >= 0x0104 && i <= 0x0133) {
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
        } else if(i >= 0x0134 && i <= 0x0143) {
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
                uint8_t val = data[i++];
                
                if(val != 0)
                    title += val;
            }
            
            std::cerr << "Title; " << title << "\n";
        } else if(i >= 0x013F && i <= 0x0142) {
            /**
             * Manufacturer code;
             * 
             * In older cartridges these bytes were part of the Title (see above).
             * In newer cartridges they contain a 4-character manufacturer code (in uppercase ASCII).
             * The purpose of the manufacturer code is unknown.
             */
            
            for(size_t j = 0; j < 4; j++)
                manufacturer += data[i+j];
            
            std::cerr << "Manufacturer: " << manufacturer << "\n";
            
            i += 4;
        } else if(i == 0x0143) {
            /**
             * CGB flag
             *
             * TODO; UNSUPORTED
             */
        } else if(i >= 0x0144 && i <= 0x0145) {
            // New licensee code
            // It is only meaningful if the Old licensee is exactly $33.
            
            std::string newLicenseeCode = "";
            
            for(size_t j = i; j < 2; j++, i++) {
                newLicenseeCode += data[j];
            }
            
            //std::cout << "Soo; " << newLicenseeCode << "\n";
        } else if(i == 0x0147) {
            // Cartridge type
            // https://gbdev.io/pandocs/The_Cartridge_Header.html#0147--cartridge-type
            
            type = getCartridgeType(data[i]);
        } else if(i == 0x0148) {
            // ROM Size
            // https://gbdev.io/pandocs/The_Cartridge_Header.html#0148--rom-size
            
            romSize = static_cast<uint8_t>(32 * (1 << data[i]));
        }  else if(i == 0x0149) {
            // RAM Size
            // https://gbdev.io/pandocs/The_Cartridge_Header.html#0149--ram-size
            
            // TODO; If data[i] == 0 - Unused aka NO RAM.
            
            ramSize = 8 * (data[i]);
        }
    }
}

CartridgeType Cartridge::getCartridgeType(uint8_t data) {
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
