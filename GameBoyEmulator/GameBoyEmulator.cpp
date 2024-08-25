#include <bitset>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>

#include "AddressBus.h"
#include "CPU.h"

/*
typedef struct {
    uint8_t entry[4];
    uint8_t logo[0x30];

    char title[16];
    uint16_t new_lic_code;
    uint8_t sgb_flag;
    uint8_t type;
    uint8_t rom_size;
    uint8_t ram_size;
    uint8_t dest_code;
    uint8_t lic_code;
    uint8_t version;
    uint8_t checksum;
    uint16_t global_checksum;
} rom_header;

typedef struct {
    char filename[1024];
    uint32_t rom_size;
    char* rom_data;
    rom_header* header;
} cart_context;

typedef enum {
    AM_IMP,
    AM_R_D16,
    AM_R_R,
    AM_MR_R,
    AM_R,
    AM_R_D8,
    AM_R_MR,
    AM_R_HLI,
    AM_R_HLD,
    AM_HLI_R,
    AM_HLD_R,
    AM_R_A8,
    AM_A8_R,
    AM_HL_SPR,
    AM_D16,
    AM_D8,
    AM_D16_R,
    AM_MR_D8,
    AM_MR,
    AM_A16_R,
    AM_R_A16
} addr_mode;

typedef enum {
    RT_NONE,
    RT_A,
    RT_F,
    RT_B,
    RT_C,
    RT_D,
    RT_E,
    RT_H,
    RT_L,
    RT_AF,
    RT_BC,
    RT_DE,
    RT_HL,
    RT_SP,
    RT_PC
} reg_type;

typedef enum {
    IN_NONE,
    IN_NOP,
    IN_LD,
    IN_INC,
    IN_DEC,
    IN_RLCA,
    IN_ADD,
    IN_RRCA,
    IN_STOP,
    IN_RLA,
    IN_JR,
    IN_RRA,
    IN_DAA,
    IN_CPL,
    IN_SCF,
    IN_CCF,
    IN_HALT,
    IN_ADC,
    IN_SUB,
    IN_SBC,
    IN_AND,
    IN_XOR,
    IN_OR,
    IN_CP,
    IN_POP,
    IN_JP,
    IN_PUSH,
    IN_RET,
    IN_CB,
    IN_CALL,
    IN_RETI,
    IN_LDH,
    IN_JPHL,
    IN_DI,
    IN_EI,
    IN_RST,
    IN_ERR,
    //CB instructions...
    IN_RLC, 
    IN_RRC,
    IN_RL, 
    IN_RR,
    IN_SLA, 
    IN_SRA,
    IN_SWAP, 
    IN_SRL,
    IN_BIT, 
    IN_RES, 
    IN_SET
} in_type;

typedef enum {
    CT_NONE, CT_NZ, CT_Z, CT_NC, CT_C
} cond_type;

struct Instruction {
    in_type type;
    addr_mode mode;
    reg_type reg_1;
    reg_type reg_2;
    cond_type cond;
    uint8_t param;

    Instruction() = default;
    Instruction(in_type t, addr_mode m, reg_type r)
        : type(t), mode(m), reg_1(r), reg_2(), cond(), param(0) {
    }

    Instruction(in_type t, addr_mode m) : type(t), mode(m) {}
    explicit Instruction(in_type in_di) : type(in_di) {}
};*/

void adc(uint8_t& a, uint8_t& reg, bool carry) {
    
}

int main(int argc, char* argv[]) {
    using std::ifstream;
    using std::ios;
    
    std::string filename = "Roms/Tennis (World).gb";
    
    ifstream stream(filename.c_str(), ios::binary | ios::ate);
    
    if (!stream.good()) {
        std::cerr << "Cannot read from file: " << filename << '\n';
    }
    
    auto fileSize = stream.tellg();
    stream.seekg(0, ios::beg);
    
    std::vector<uint8_t> data(fileSize);

    if (!stream.read(reinterpret_cast<char*>(data.data()), fileSize)) {
        std::cerr << "Error reading file!" << '\n';
        
        return 1;
    }

    // Init CPU
    CPU cpu;
    
    // Init Address Bus
    // TODO; Move into CPU
    AddressBus bus(data);
    
    // Code from here is a mere test.
    // TODO; Remove
    
    // Gather cartridge information
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
            
            std::string title;
            
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
            
            std::string manufacturer;
            
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
            
            uint8_t type = data[i];
            printf("Type; %x\n", type);
            std::cerr << "\n";
            
            // ROM ONLY - 0
        } else if(i == 0x0148) {
            // ROM Size
            // https://gbdev.io/pandocs/The_Cartridge_Header.html#0148--rom-size
            
            uint8_t romSize = 32 * (1 << data[i]);
            printf("Size; %d\n", romSize);
            std::cerr << "\n";
        }  else if(i == 0x0149) {
            // RAM Size
            // https://gbdev.io/pandocs/The_Cartridge_Header.html#0149--ram-size
            
            // TODO; If data[i] == 0 - Unused aka NO RAM.
            
            uint8_t ramSize = 8 * (data[i]);
            printf("Size; %d\n", ramSize);
            std::cerr << "\n";
        }
    }
    
    /*cart_context ctx = cart_context();
    
    ctx.rom_data = new char[file_size];
    stream.read(ctx.rom_data, file_size);
    
    ctx.header = (rom_header*)(ctx.rom_data + 0x100);
    
    AddressBus bus;
    
    Instruction instructions[0x100] = {
        Instruction(in_type::IN_NOP, addr_mode::AM_IMP), // Index 0x00
        Instruction(in_type::IN_DEC, addr_mode::AM_R, RT_B), // Index 0x05
        Instruction(in_type::IN_LD, addr_mode::AM_R_D8, RT_C), // Index 0x0E
        Instruction(in_type::IN_XOR, addr_mode::AM_R), // Index 0xAF
        Instruction(in_type::IN_JP, addr_mode::AM_D16), // Index 0xC3
        Instruction(in_type::IN_DI), // Index 0xF3
    };*/
    
    while(true) {
        uint16_t opcode = bus.fetch8(cpu.PC++);
        
        switch (opcode) {
            case 0x00: {
                // NOP
                break;
            }
            
            case 0x01: {
                /* 
                 * LD BC, D16
                 * 3, 12
                 *
                 */
                
                uint16_t d16 = bus.fetch16(cpu.PC);
                cpu.registers.BC = d16;
                
                cpu.PC += 2;
                
                break;
            }

            case 0x2A: {
                /** TODO; WRONG
                 * LD HL, d16
                 * 3, 12
                 */
                
                uint16_t d16 = bus.fetch16(cpu.PC);
                cpu.registers.HL = d16;
                
                cpu.PC += 2;
                
                break;
            }
            
            case 0x06: {
                /** TODO; WRONG
                 * LD B, D8
                 * 2, 8
                 */
                
                uint8_t d8 = bus.fetch8(cpu.PC++);
                cpu.registers.BC.B = d8;
                
                break;
            }
            
            case 0xC3: {
                /**
                 * JP a16
                 * 3, 16
                 */
                
                uint16_t address = bus.fetch16(cpu.PC++);
                
                // Jump to 'address' unconditionally
                cpu.PC = address;
                
                break;
            }
            
            case 0x31: {
                /*
                 * LD SP, d16
                 * 3, 12
                 *
                 *  Jump to the absolute address specified by the next two bytes
                 */
                
                /*uint8_t low = bus.fetch8(cpu.PC);
                uint8_t high = bus.fetch8(cpu.PC + 1);
                uint16_t f = (low | high) >> 8;
                /*printf("Low; %x - High: %x | together = %x << 8 = %x\n", low, high, low | high, (low | high) << 8);
                std::cerr << "";#1#
                
                uint16_t fj = bus.fetch16(cpu.PC);*/
                cpu.SP = bus.fetch16(cpu.PC++);
                
                break;
            }
            
            case 0xDF: { // RST 18H
                // Push the current PC onto the stack
                cpu.SP -= 2;
                bus.write16(cpu.SP, cpu.PC);        // Push lower byte
                
                // Jump to the address 0x0038
                cpu.PC = 0x18;
                
                break;
            }
            
            case 0xFF: { // RST 38H
                // Push the current PC onto the stack
                cpu.SP -= 2;
                bus.write16(cpu.SP, cpu.PC);        // Push lower byte
                
                // Jump to the address 0x0038
                cpu.PC = 0x38;
                
                break;
            }
            
            case 0x39: {
                /**
                 * ADD HL, SP
                 * 1, 8
                 * - 0 H C
                 */
                
                uint32_t results = cpu.registers.HL + cpu.SP;
                cpu.registers.HL = results;
                
                cpu.flags.C = (results > 0xFFFF);
                
                
                
                cpu.flags.N = 0;
                
                break;
            }

            case 0xE1: {
                /**
                 * POP HL,
                 * 1, 12
                 */
                
                uint16_t val = bus.fetch16(cpu.SP);
                
                cpu.registers.HL.H = (val >> 8) & 0xFF;
                cpu.registers.HL.L = val & 0xFF;
                
                cpu.SP += 2;
                
                break;
            }
            
            // ld reg, reg
            case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x47: // ld b,reg
            case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4f: // ld c,reg
            case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x57: // ld d,reg
            case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5f: // ld e,reg
            case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x67: // ld h,reg
            case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6f: // ld l,reg
            case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7f: { // ld a,reg
                uint8_t& dst = cpu.registers.getRegister(opcode >> 3 & 7);
                uint8_t src = cpu.registers.getRegister(opcode & 7);
                
                dst = src;
                
                /*uint8_t& dst = regs[opcode >> 3 & 7];
                u8 src = regs[opcode & 7];
                dst = src;*/
                
                break;
            }
            
            case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x87: // add a,reg
            case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8f: { // adc a,reg
                bool carry = (opcode & 8) && cpu.registers.AF.getCarry();
                
                adc(cpu.registers.AF.A, cpu.registers.getRegister(opcode & 7), carry);
                
                break;
            }
            
            default: {
                /*
                std::bitset<8> x(opcode);
                std::cout << x << '\n';
                */
                
                printf("Code; %x\n", opcode);
                std::cerr << "";
                
                break;
            }
        }
        
        /*switch (opcode) {
        case 0x00:
            // NOP
            break;
        
        case 0x02: {
            /**
             * LD (BC), A
             * 1, 8
             #1#
            
            cpu.registers.BC = cpu.registers.AF.A;
            
            break;
        }
        case 0x31: {
            /*
             * LD SP, d16
             * 3, 12
             *
             *  Jump to the absolute address specified by the next two bytes
            #1#
            
            /*uint8_t low = bus.fetch8(pc);
            uint8_t high = bus.fetch8(pc + 1);
            
            printf("Low; %x - High: %x | together = %x << 8 = %x\n", low, high, low | high, (low | high) << 8);
            std::cerr << "";#1#
            
            cpu.registers.SP = bus.fetch16(cpu.registers.PC++);
            
            break;
        }
        case 0x76:
            // HALT
            std::cerr << "HALT" << '\n';
            
            break;
        case 0xC3: {
            /**
             * JP a16
             * 3, 16
             #1#
            
            uint16_t address = bus.fetch16(cpu.registers.PC++);
            
            // Jump to 'address' unconditionally
            cpu.registers.PC = address;
            
            break;
        }
        default: {
            /*std::bitset<8> x(opcode);
            std::cout << x << '\n';#1#
            
            printf("Code; %x\n", opcode);
            std::cerr << "\n";
            
            break;
        }
        }*/
    }
    
    return 0;
}
