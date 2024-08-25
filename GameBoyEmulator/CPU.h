#pragma once
#include <cstdint>
#include <iostream>

class CPU {
public:
    /**
     * According to https://gbdev.io/pandocs/CPU_Registers_and_Flags.html
     * CPU contains 8 registers; every register seems to be a u8,
     * with it being combined to become a u16;
     * 
     * Copied refal table from; https://gbdev.io/pandocs/CPU_Registers_and_Flags.html
     * 16-bit   Hi  Lo  Name/Function
     * AF       A   -   Accumulator & Flags
     * BC       B   C   BC
     * DE       D   E   DE
     * HL       H   L   HL
     * SP       -   -   Stack Pointer
     * PC       -   -   Program Counter/Pointer
     * 
     * "As shown above, most registers can be accessed either as one 16-bit register,
     *      or as two separate 8-bit registers." - https://gbdev.io/pandocs/CPU_Registers_and_Flags.html
     * 
     * To achieve such a behaviour in C++, I will be using union.
     */

    union Registers {
        struct AF {
            uint8_t A; // Flags register
            uint8_t F; // Accumulator register
            
            inline bool getCarry() {
                // Extracts the carry from the F register flag
                return F & 0x10f;
            }
        } AF;

        struct BC {
            uint8_t B;
            uint8_t C;
            
            BC& operator=(uint16_t value) {
                B = static_cast<uint8_t>(((value >> 8) & 0xFF));
                C = static_cast<uint8_t>(value & 0xFF);
                
                return *this;
            }
        } BC;

        struct DE {
            uint8_t D;
            uint8_t E;
        } DE;
        
        struct HL {
            uint8_t H;
            uint8_t L;
            
            HL& operator=(uint16_t value) {
                H = static_cast<uint8_t>(((value >> 8) & 0xFF));
                L = static_cast<uint8_t>(value & 0xFF);
                
                return *this;
            }
            
            HL& operator+=(uint16_t value) {
                H += static_cast<uint8_t>(((value >> 8) & 0xFF));
                L += static_cast<uint8_t>(value & 0xFF);
                
                return *this;
            }
            
            uint16_t operator+(uint16_t value) const {
                uint16_t results = H + static_cast<uint8_t>(((value >> 8) & 0xFF));
                results += L + static_cast<uint8_t>((value & 0xFF));
                
                return results;
            }
        } HL;
        
        uint8_t& getRegister(uint16_t reg) {
            switch (reg) {
                case 0: {
                    return BC.B;
                }
                case 1: {
                    return BC.C;
                }
                case 2: {
                    return DE.D;
                }
                case 3: {
                    return DE.E;
                }
                case 4: {
                    return HL.H;
                }
                case 5: {
                    return HL.L;
                }
                case 6: {
                    return AF.A;
                }
                case 7: {
                    return AF.F;
                }
            default:
                std::cerr << "[CPU-getRegister] Unknown register; " << reg << "\n";
                
                break;
            }
        }
    };

    /**
     * Flag Register (lower 8 bits of AF register) - https://gbdev.io/pandocs/CPU_Registers_and_Flags.html
     * 
     * Copied refal table from; https://gbdev.io/pandocs/CPU_Registers_and_Flags.html
     * Bit  Name    Explanation
     * 7    z       Zero flag
     * 6    n       Subtraction flag (BCD)
     * 5    h       Half Carry flag (BCD)
     * 4    c       Carry flag
     * 
     * According to https://gbdev.io/pandocs/CPU_Registers_and_Flags.html;
     * "Contains information about the result of the most recent instruction that has affected flags."
     * 
     * Reset copied information from https://gbdev.io/pandocs/CPU_Registers_and_Flags.html;
     * 
     * The Zero Flag (Z)
     * This bit is set if and only if the result of an operation is zero. Used by conditional jumps.
     * 
     * The Carry Flag (C, or Cy)
     * Is set in these cases:
     * 
     * When the result of an 8-bit addition is higher than $FF.
     * When the result of a 16-bit addition is higher than $FFFF.
     * When the result of a subtraction or comparison is lower than zero (like in Z80 and x86 CPUs, but unlike in 65XX and ARM CPUs).
     * When a rotate/shift operation shifts out a “1” bit.
     * Used by conditional jumps and instructions such as ADC, SBC, RL, RLA, etc.
     * 
     * The BCD Flags (N, H)
     * These flags are used by the DAA instruction only.
     * N indicates whether the previous instruction has been a subtraction,
     * and H indicates carry for the lower 4 bits of the result.
     * DAA also uses the C flag, which must indicate carry for the upper 4 bits.
     * After adding/subtracting two BCD numbers, DAA is used to convert the result to BCD format.
     * BCD numbers range from $00 to $99 rather than $00 to $FF.
     * Because only two flags (C and H) exist to indicate carry-outs of BCD digits,
     * DAA is ineffective for 16-bit operations (which have 4 digits),
     * and use for INC/DEC operations (which do not affect C-flag) has limits.
     *
     * Please note most of this information is copied from; https://gbdev.io/pandocs/CPU_Registers_and_Flags.html
     * as this website will be used as a guide to complete this project.
     */
    /*enum FlagRegister {
        Z = 1 << 7, // Zero Flag
        N = 1 << 6, // Subtract Flag
        H = 1 << 5, // Half Carry Flag
        C = 1 << 4  // Carry Flag
    };*/
    
    struct Flags {
        uint8_t Z = 1 << 7; // Zero Flag
        uint8_t N = 1 << 6; // Subtract Flag
        uint8_t H = 1 << 5; // Half Carry Flag
        uint8_t C = 1 << 4; // Carry Flag
    };
    
public:
    void reset();
    
public:
    Registers registers;

    /**
     * Flags in order;
     *
     * Z N H C
     */
    Flags flags;
    
    // Program Counter/Pointer
    uint16_t PC = 0x0100;
    
    // Stack Pointer
    uint16_t SP = 0x0;
};
