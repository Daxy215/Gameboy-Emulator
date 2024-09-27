#pragma once

#include <cstdint>
#include <iostream>

class InterruptHandler;

class MMU;

class CPU {
public:
    CPU(InterruptHandler& interruptHandler, MMU& mmu);
    
    uint16_t fetchOpCode();
    uint16_t decodeInstruction(uint16_t opcode);
    uint16_t decodePrefix(uint16_t opcode);

    void bootRom();
    
    void popReg(uint8_t& reg);
    void popReg(uint8_t& high, uint8_t& low);
    
    void jr(int8_t& offset);
    
    int jrc(int8_t& offset);
    int jrnc(int8_t& offset);
    
    int jpc(uint16_t& address);
    int jpnc(uint16_t& address);

    int callc(uint16_t& address);
    int callnc(uint16_t& address);
    
    int jrz(int8_t& offset);
    int jrnz(int8_t& offset);

    int jpz(uint16_t& address);
    int jpnz(uint16_t& address);
    
    int callz(uint16_t& address);
    int callnz(uint16_t& address);
    
    void ld(uint8_t& regA, uint8_t& regB);
    
    void rra();
    void rst(uint16_t pc);
    
    void adc(uint8_t& a, const uint8_t& reg, bool carry);
    void add(uint8_t& regA, uint8_t& regB);
    void add(uint16_t& regA, uint16_t& regB);
    
    void sbc(uint8_t& regA, const uint8_t& regB, bool carry);
    void sub(uint8_t& regA, uint8_t& regB);
    
    void inc(uint8_t& reg);
    void dec(uint8_t& reg);
    
    void xor8(uint8_t& regA, uint8_t& regB);
    
    // Prefix instructions
    void checkBit(uint8_t bit, uint8_t& reg);
    void checkBit(uint8_t bit, uint16_t& reg);
    
    void clearBit(uint8_t bit, uint8_t& reg);
    void clearBit(uint8_t bit, uint16_t& reg);
    
    void setBit(uint8_t bit, uint8_t& reg);
    void setBit(uint8_t bit, uint16_t& reg);
    
    void swap(uint8_t& reg) noexcept;
    
    void rlc(uint8_t& reg);
    void rlc(uint16_t& reg);
    
    void rl(uint8_t& reg);
    void rrc(uint8_t& reg);
    void rr(uint8_t& reg);
    
    void srl(uint8_t& reg);
    void sla(uint8_t& reg);
    void sra(uint8_t& reg);
    
    void pushToStack(uint16_t value);
    uint16_t popStack();
    
    void reset();

public:
    InterruptHandler& interruptHandler;
    
    MMU& mmu;
    
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
    
    //union Registers {
        struct AF {
            uint8_t A; // Flags register
            uint8_t F; // Accumulator register
            
            AF& operator=(uint16_t value) {
                A = static_cast<uint8_t>((value >> 8) & 0xFF);
                F = static_cast<uint8_t>(value & 0xFF);
                
                return *this;
            }
            
            inline bool getCarry() const {
                // Extracts the carry from the F register flag
                return F & Flags::C;
            }

            inline void setCarry(bool flag) {
                if (flag) F |= Flags::C;
                else F &= ~Flags::C;
            }
            
            inline bool getZero() const {
                return F & Flags::Z;
            }
            
            inline void setZero(bool flag) {
                if (flag) F |= Flags::Z;
                else F &= ~Flags::Z;
            }
            
            inline bool getHalfCarry() const {
                return F & Flags::H;
            }
            
            inline void setHalfCarry(bool flag) {
                if (flag) F |= Flags::H;
                else F &= ~Flags::H;
            }
            
            inline bool getSubtract() const {
                return F & Flags::N;
            }
            
            inline void setSubtract(bool flag) {
                if (flag) F |= Flags::N;
                else F &= ~Flags::N;
            }
            
            uint16_t get() const {
                return static_cast<uint16_t>((A << 8) | F);
            }
        } AF;
        
        struct BC {
            uint8_t B;
            uint8_t C;
            
            BC& operator=(uint16_t value) {
                B = static_cast<uint8_t>((value >> 8) & 0xFF);
                C = static_cast<uint8_t>(value & 0xFF);
                
                return *this;
            }
            
            BC& operator+=(uint16_t value) {
                uint16_t combined = (static_cast<uint16_t>((B << 8) | C));
                combined += value;
                
                *this = combined;
                return *this;
            }
            
            uint16_t get() const {
                return (static_cast<uint16_t>((B << 8) | C));
            }
        } BC;
        
        struct DE {
            uint8_t D;
            uint8_t E;
            
            DE& operator=(uint16_t value) {
                D = static_cast<uint8_t>(((value >> 8) & 0xFF));
                E = static_cast<uint8_t>(value & 0xFF);
                
                return *this;
            }
            
            uint16_t operator+(uint16_t value) const {
                return static_cast<uint16_t>(D << 8 | E) + value;
            }
            
            uint16_t get() {
                return static_cast<uint16_t>((D << 8) | E);
            }
        } DE;
        
        struct HLReg {
            uint8_t H;
            uint8_t L;
            
            HLReg& operator=(uint16_t value) {
                H = static_cast<uint8_t>(((value >> 8) & 0xFF));
                L = static_cast<uint8_t>(value & 0xFF);
                
                return *this;
            }
            
            HLReg& operator+=(uint16_t value) {
                uint16_t newL = L + (value & 0xFF);
                
                H += (value >> 8) + (newL >> 8);
                L = newL & 0xFF;
                
                return *this;
            }
            
            uint16_t operator+(uint16_t value) const {
                return static_cast<uint16_t>(H << 8 | L) + value;
            }
            
            /*operator uint16_t() const {
                return static_cast<uint16_t>(H << 8 | L);
            }*/
            
            uint16_t get() const {
                return static_cast<uint16_t>((H << 8) | L);
            }
        } HL;
        
        uint8_t& getRegister(uint16_t reg) {
            switch (reg) {
            case 0: return BC.B;
            case 1: return BC.C;
            case 2: return DE.D;
            case 3: return DE.E;
            case 4: return HL.H;
            case 5: return HL.L;
            case 6: return AF.F;
            case 7: return AF.A;
            default:
                std::cerr << "[CPU-getRegister] Unknown register; " << reg << "\n";
                   
                break;
            }
        }
    //};
    
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
     *
     * FLAG_Z: If after the addition the result is 0
     * FLAG_N: Set to 0
     * FLAG_C: If the result will be greater than max byte (255)
     * FLAG_H: If there will be a carry from the lower nibble to the upper nibble
     */
    
    struct Flags {
        static const uint8_t Z = 1 << 7; // Zero Flag
        static const uint8_t N = 1 << 6; // Subtract Flag
        static const uint8_t H = 1 << 5; // Half Carry Flag
        static const uint8_t C = 1 << 4; // Carry Flag
    };
    
public:
    bool halted = false;
    bool stop;
    
    int8_t ei = -1;
    
    // Program Counter/Pointer
    uint16_t PC = 0x0100;
    
    // Stack Pointer
    uint16_t SP = 0xFFFE;
};
