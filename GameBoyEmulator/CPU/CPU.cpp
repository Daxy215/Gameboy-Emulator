#include "CPU.h"

#include <cassert>

#include "InterrupHandler.h"

#include "../Memory/MMU.h"
#include "../Utility/Bitwise.h"

CPU::CPU(InterruptHandler& interruptHandler, MMU& mmu)
    : interruptHandler(interruptHandler), mmu(mmu) {
    // https://gbdev.io/pandocs/Power_Up_Sequence.html#cpu-registers

	// Expected: A: 11 F: 80 B: 00 C: 00 D: FF E: 56 H: 00 L: 0D SP: FFFE PC: 00:0100 (00 C3 13 02)
	AF.A = 0x11;
	AF.F = 0x80;
	BC.B = 0x00;
	BC.C = 0x00;
	DE.D = 0xFF;
	DE.E = 0x56;
	HL.H = 0x00;
	HL.L = 0x0D;
	
    /*AF.A = 0x01;
    AF.F = 0xB0;
    BC.B = 0x00;
    BC.C = 0x13;
    DE.D = 0x00;
    DE.E = 0xD8;
    HL.H = 0x01;
    HL.L = 0x4D;*/
    
    PC = 0x0100;
    SP = 0xFFFE;
}

uint16_t CPU::fetchOpCode() {
    uint16_t opcode = mmu.fetch8(PC++);
    
    return opcode;
}

int CPU::decodeInstruction(uint16_t opcode) {
    switch (opcode) {
        case 0x00: {
            // NOP
            // 1, 4
            
            return 4;
        }
        
        case 0x01: {
            /* 
             * LD BC, D16
             * 3, 12
             *
             */
            
            uint16_t d16 = mmu.fetch16(PC);
            BC = d16;
            
            PC += 2;
            
            return 12;
        }
		
        case 0x02: {
            /**
             * LD (BC), A
             * 1, 8
             */
            
            mmu.write8(BC.get(), AF.A);
            
            return 8;
        }
        
        case 0x03: {
            /**
             * INC BC
             * 1, 8
             * - - - -
             */
            
            BC += 1;
            
            return 8;
        }
		
		case 0x04: {
	        /**
	         * INC B
	         * 1, 4
	         */
			
        	inc(BC.B);
        	
        	return 4;
        }
		
		case 0x05: {
			/**
			 * DEC B
			 * 1, 4
			 * Z 1 H -
			 */
			
        	dec(BC.B);
        	
        	return 4;
        }
        
        case 0x06: {
            /**
             * LD B, D8
             * 2, 8
             */
            
            uint8_t d8 = mmu.fetch8(PC++);
            BC.B = d8;
            
            return 8;
        }

		case 0x07: {
	        /**
	         * RLCA
	         * 1, 4
	         * 0 0 0 C
	         */
			
        	uint8_t value = AF.A;
        	
        	uint8_t bit7 = (value >> 7) & 1;
        	value = (value << 1) | bit7;
			
        	AF.A = value;
			
        	AF.setZero(false);
        	AF.setSubtract(false);
        	AF.setHalfCarry(false);
        	AF.setCarry(bit7 == 1);

        	return 4;
        }
		
		case 0x08: {
			/**
			 * LD (u16), SP
			 * 3, 20
			 * - - - -
			 */
			
        	uint16_t u16 = mmu.fetch16(PC);
        	
        	// TODO; Maybe use push to stack??
        	mmu.write8(u16, SP & 0xFF);
        	mmu.write8(u16 + 1, (SP >> 8) & 0xFF);
			
        	PC += 2;
        	
        	return 20;
        }
		
		/*case 0x09: {
			/**
			 * 
			 #1#
		}*/
		
		case 0x0B: {
			/**
			 * DEC BC
			 * 1, 8
			 * - - - -
			 */
			
        	BC = BC.get() - 1;
			
        	return 8;
        }
		
		case 0x0C: {
			/**
			 * INC C
			 * 1, 4
			 * Z 1 H -
			 */
			
        	inc(BC.C);
			
        	return 4;
        }
    	
		case 0x0D: {
			/**
			 * DEC C
			 * 1, 4
			 * Z 1 H -
			 */
			
        	dec(BC.C);
			
        	return 4;
        }
        
        case 0x0E: {
            /**
             * LD C, u8
             * 2, 8
             */
            
            BC.C = mmu.fetch8(PC++);
            
            return 8;
        }
		
		case 0xF: {
			/**
			 * RRCA (Roate A right, through carry)
			 * 1, 4
			 * 0 0 0 C
			 */
			
        	// Save the least significant bit (bit 0)
        	uint8_t bit0 = AF.A & 0x01;
			
        	// Shift the register A right by 1, and put the bit 0 into bit 7
        	AF.A = (AF.A >> 1) | (bit0 << 7);
			
        	AF.setZero(false);
        	AF.setSubtract(false);
        	AF.setHalfCarry(false);
        	AF.setCarry(bit0 == 1);
        	
        	return 4;
        }
		
		case 0x10: {
			/**
			 * STOP
			 * 1, 4
			 * - - - -
			 */

        	// TODO;
        	stop = true;
        	
        	return 4;
        }
    	
        case 0x11: {
            /**
             * LD DE, d16
             * 3, 12
             */
            
            DE = mmu.fetch16(PC);
            
            PC += 2;
            
            return 12;
        }
		
        case 0x12: {
            /**
             * LD (DE), A
             * 1, 8
             */
        	
        	//uint16_t address = mmu.fetch16(DE.get());
            mmu.write8(DE.get(), AF.A);
            
            return 8;
        }
		
		case 0x13: {
			/**
			 * INC DE
			 * 1, 8
			 * - - - -
			 */
			
        	DE = DE.get() + 1;
        	
        	return 8;
        }
		
		case 0x14: {
			/**
			 * INC D
			 * 1, 4
			 * Z 0 H -
			 */
			
        	inc(DE.D);
			
        	return 4;
        }
		
    	case 0x15: {
			/**
			 * DEC D
			 * 1, 4
			 * Z 0 H -
			 */
			
        	dec(DE.D);
			
        	return 4;
        }

		case 0x17: {
			/**
			 * RLA (Rotate Left A throguh Carry)
			 * 1, 4
			 * 0 0 0 C
			 */
			
        	bool carry = AF.getCarry();
        	bool msb = (AF.A & 0x80) != 0;
			
        	AF.A = (AF.A << 1) | (carry ? 1 : 0);
			
        	AF.setCarry(msb);
        	AF.setZero(false);
        	AF.setSubtract(false);
        	AF.setHalfCarry(false);
        	
        	return 4;
        }
    	
        case 0x18: {
            /**
             * JR i8
             * 2, 12
             */
            
            int8_t i8 = static_cast<int8_t>(mmu.fetch8(PC));
			jr(i8);
        	
            //PC += i8;
            
            return 12;
        }

		case 0x1A: {
			/**
			 * LD A, (DE)
			 * 1, 8
			 * - - - - 
			 */
			
        	AF.A = mmu.fetch8(DE.get());
        	
        	return 8;
        }

		case 0x1B: {
			/**
			 * DEC DE
			 * 1, 8
			 * - - - -
			 */
			
        	DE = DE.get() - 1;
        	
        	return 8;
        }
        
        case 0x1C: {
            /**
             * INC E
             * 1, 4
             * Z N H -
             */
			
			inc(DE.E);
        	
            return 4;
        }
		
		case 0x1D: {
			/**
			 * DEC E
			 * 1, 4
			 * z 1 H -
			 */
			
        	dec(DE.E);
        	
        	return 4;
        }
		
		case 0x1F: {
        	/**
        	 * RRA
        	 * 1, 4
        	 * 0 0 0 C
        	 */
			
        	rra();
        	return 4;
        }
        
        case 0x20: {
            /*
             * JR NZ, i8
             * 2, (8 without a branch, 12 with branch)
             *
             * Taken from; https://meganesu.github.io/generate-gb-opcodes/
             * If the Z flag is 0, jump s8 steps from the current address stored in the program counter (PC).
             * If not, the instruction following the current JP instruction is executed (as usual).
             */
			
        	int8_t offset = static_cast<int8_t>(mmu.fetch8(PC));
        	return jrnz(offset);
        	
            /*if (!AF.getZero()) {
                
                PC += offset;
                
                return 12;
            }
            
            PC++; // To skip the offset byte
            
            return 8;*/
        }
        
        case 0x21: {
            /**
             * LD HL, u16
             * 3, 12
             */
            
            HL = mmu.fetch16(PC);
            
            PC += 2;
            
            return 12;
        }
		
		case 0x22: {
			/**
			 * LD (HL+), A
			 * 1, 8
			 * - - - -
			 */
			
        	mmu.write8(HL.get(), AF.A);
        	HL = HL.get() + 1;
			
        	return 8;
        }
		
        case 0x23: {
            /**
             * INC HL
             * 1, 8
             */
            
            HL = (HL + 1);
            
            return 8;
        }
		
		case 0x24: {
			/**
			 * INC H
			 * 1, 4
			 * Z 0 H -
			 */
			
        	inc(HL.H);
        	
        	return 4;
        }
		
		case 0x25: {
			/**
			 * DEC H
			 * 1, 4
			 * Z 1 H -
			 */
			
        	dec(HL.H);
        	
        	return 4;
        }
		
		case 0x26: {
	        /**
	         * LD H, u8
	         * 2, 8
	         * - - - -
	         */
			
        	uint8_t u8 = mmu.fetch8(PC++);
        	ld(HL.H, u8);
        	
        	return 8;
        }
		
		case 0x28: {
			/**
			 * JR Z, i8
			 * 2, 8 (12 with branch)
			 */
			
        	int8_t i8 = static_cast<int8_t>(mmu.fetch8(PC));
        	return jrz(i8);
        	
        	/*if(AF.getCarry()) {
        		int8_t i8 = static_cast<int8_t>(mmu.fetch8(PC));
				
        		PC += i8;
				
        		return 12;
        	}
			
        	PC++;*/
        	
        	return 8;
        }
		
		case 0x29: {
	        /**
	         * ADD HL, HL
	         * 1, 8
	         * - 0 H C
	         */
			
        	uint16_t hl = HL.get();
        	uint32_t res = hl + hl;
			
        	AF.setSubtract(false);
        	AF.setHalfCarry(((hl & 0x0FFF) * 2) > 0x0FFF);
        	AF.setCarry(res > 0xFFFF);
			
        	HL = static_cast<uint16_t>(res & 0xFFFF); // Lower 16 bits
        	
        	return 8;
        }
    	
    	case 0x2A: {
            /**
             * LD A, (HL+)
             * 1, 8
             */
			
        	/*if(PC == 0x0207) {
        		printf("LD A (HL+)");
        	}*/
        	
            uint16_t address = HL.get();
            AF.A = mmu.fetch8(address);
			
        	// Had an issue with this having the wrong implementation
            HL += 1;
            
            return 8;
        }
		
		case 0x2C: {
	        /**
	         * INC L
	         * 1, 4
	         * Z 0 H -
	         */
        	
			inc(HL.L);
        	
        	return 4;
        }
    	
		case 0x2E: {
	        /**
	         * LD L, u8
	         * 2, 8
	         * - - - -
	         */
			
        	HL.L = mmu.fetch8(PC++);
        	
        	return 8;
        }
		
		case 0x2D: {
			/**
			 * DEC L
			 * 1, 4
			 * Z 1 H -
			 */
			
        	dec(HL.L);
        	
        	return 4;
        }
		
		case 0x2F: {
			/**
			 * CPL - Complement A
			 * 1, 4
			 * - 1 1 -
			 */
			
        	AF.A = ~AF.A;
			
        	AF.setSubtract(true);
        	AF.setHalfCarry(true);
        	
        	return 4;
        }
		
		case 0x30: {
			/**
			 * JR NC, i8
			 * 2, 8 (12 with branch)
			 * - - - -
			 */
			
        	int8_t i8 = static_cast<int8_t>(mmu.fetch8(PC));
        	return jrnc(i8);
        }
        
        case 0x31: {
            /*
             * LD SP, d16
             * 3, 12
             *
             *  Jump to the absolute address specified by the next two bytes
             */
            
            SP = mmu.fetch16(PC);
            
            PC += 2;
            
            return 12;
        }
		
		case 0x32: {
			/**
			 * LD (HL-), A
			 * 1, 8
			 * - - - -
			 */
			
        	uint16_t address = HL.get();
        	mmu.write8(address, AF.A);
        	HL = HL.get() - 1;
        	
        	return 8;
        }

		case 0x33: {
			/**
			 * INC SP
			 * 1, 8
			 * - - - -
			 */
			
        	SP++;
        	
        	return 8;
        }
		
        case 0x34: {
            /**
             * INC (HL)
             * 1, 12
             * Z 0 H -
             */
			
        	uint16_t address = HL.get();
            uint8_t val = mmu.fetch8(address);
            uint8_t result = val + 1;
			
        	mmu.write8(address, result);
			
        	AF.setZero(result == 0);
        	AF.setSubtract(false);
        	AF.setHalfCarry(((val & 0x0F) + 1) > 0x0F);
            
            return 12;
        }
		
		case 0x35: {
			/**
			 * DEC (HL)
			 * 1, 12
			 * Z 1 H -
			 */
			
        	uint8_t val = mmu.fetch8(HL.get());
        	uint8_t res = val - 1;
			
        	mmu.write8(HL.get(), res);
			
        	AF.setZero(res == 0);
        	AF.setSubtract(true);
        	AF.setHalfCarry((res & 0x0F) == 0x00);
        	
        	return 12;
        }
		
		case 0x37: {
			/**
			 * SCF
			 * 1, 4
			 * - 0 0 1
			 */
			
        	AF.setSubtract(false);
        	AF.setHalfCarry(false);
        	AF.setCarry(true);
        	
        	return 4;
        }
		
		case 0x38: {
			/**
			 * JR C, i8
			 * 2, 8 (12 with branch)
			 * - - - -
			 */
			
        	int8_t i8 = static_cast<int8_t>(mmu.fetch8(PC));
        	return jrc(i8);
        	
        	//return 8;
        }
        
        case 0x39: {
            /**
             * ADD HL, SP
             * 1, 8
             * - 0 H C
             */
            
        	uint16_t HL_value = HL.get();
        	uint16_t SP_value = SP;
			
        	// Perform the addition
        	uint32_t result = HL_value + SP_value;
        	
        	HL = static_cast<uint16_t>(result); 
			
        	AF.setSubtract(false);
        	AF.setHalfCarry(((HL_value & 0x0FFF) + (SP_value & 0x0FFF)) > 0x0FFF);
        	AF.setCarry(result > 0xFFFF);
            
            return 8;
        }
		
		case 0x3B: {
			/**
			 * DEC SP
			 * 1, 8
			 * - - - -
			 */
        	
			SP--;	
        	
        	return 8;
        }
		
		case 0x3C: {
        	/**
			 * INC A
			 * 1, 4
			 * Z 0 H -
			 */
            
        	inc(AF.A);
            
        	return 4;
        }
		
		case 0x3D: {
			/**
			 * DEC A
			 * 1, 4
			 * Z 1 H -
			 */
			
        	dec(AF.A);
        	
        	return 4;
        }
		
    	 case 0x3E: {
            /**
             * LD A, u8
             * 2, 8
             */
            
            AF.A = mmu.fetch8(PC++);
            
            return 8;
        }
		
		case 0x3F: {
			/**
			 * CCF (Complement Carry Flag)
			 * 1, 4
			 * - 0 0 -
			 */
			
        	AF.setCarry(!AF.getCarry());
			
        	AF.setSubtract(false);
        	AF.setHalfCarry(false);
        	
        	return 4;
        }
		
		case 0x46: {
	        /**
	         * LD B, (HL)
	         * 1, 8
	         * - - - -
	         */
			
        	uint8_t u8 = mmu.fetch8(HL.get());
        	/*printf("%x %x\n", HL.get(), mmu.fetch8(HL.get()));
        	std::cerr << "";*/
        	ld(BC.B, u8);
        	
        	return 8;
        }
		
		case 0x4E: {
			/**
			 * LD C, (HL)
			 * 1, 8
			 * - - - -
			 */
			
        	BC.C = mmu.fetch8(HL.get());
			
        	return 8;
        }
		
		case 0x56: {
			/**
			 * LD D, (HL)
			 * 1, 8
			 * - - - -
			 */
			
        	DE.D = mmu.fetch8(HL.get());
        	
        	return 8;
        }

		case 0x5E: {
			/**
			 * LD E, (HL)
			 * 1, 8
			 * - - - -
			 */
			
        	DE.E = mmu.fetch8(HL.get());
        	
        	return 8;
        }

		case 0x66: {
			/**
			 * LD H, (HL)
			 * 1, 8
			 * - - - -
			 */
        	
        	HL.H = mmu.fetch8(HL.get());
        	
        	return 8;
        }
		
		case 0x6E: {
			/**
			 * LD L, (HL)
			 * 1, 8
			 * - - - -
			 */
			
        	uint8_t u8 = mmu.fetch8(HL.get());
        	ld(HL.L, u8);
        	
        	return 8;
        }
		
    	case 0x70: {
			/**
			 * LD (HL), B
			 * 1, 8
			 * - - - -
			 */
			
        	mmu.write8(HL.get(), BC.B);
        	
        	return 8;
        }
		
		case 0x71: {
			/**
			 * LD (HL), C
			 * 1, 8
			 * - - - -
			 */
			
        	mmu.write8(HL.get(), BC.C);
        	
        	return 8;
        }
    	
		case 0x72: {
			/**
			 * LD (HL), D
			 * 1, 8
			 * - - - -
			 */
			
        	mmu.write8(HL.get(), DE.D);
        	
        	return 8;
        }
		
		case 0x73: {
			/**
			 * LD (HL), E
			 * 1, 8
			 * - - - -
			 */
			
        	mmu.write8(HL.get(), DE.E);
        	
        	return 8;
        }
		
    	case 0x74: {
			/**
			 * LD (HL), H
			 * 1, 8
			 * - - - -
			 */
			
        	mmu.write8(HL.get(), HL.H);
        	
        	return 8;
        }
    	
    	case 0x75: {
			/**
			 * LD (HL), H
			 * 1, 8
			 * - - - -
			 */
			
        	mmu.write8(HL.get(), HL.L);
        	
        	return 8;
        }
    	
		case 0x77: {
			/**
			 * LD (HL), A
			 * 1, 8
			 * - - - -
			 */
			
        	//printf("%x %x\n", HL.get(), AF.A);
        	//std::cerr << "";
			
        	mmu.write8(HL.get(), AF.A);
        	
        	return 8;
        }
		
		case 0x7E: {
			/**
			 * LD A, (HL)
			 * 1, 8
			 * - - - -
			 */
			
        	AF.A = mmu.fetch8(HL.get());
        	
        	return 8;
        }
		
		case 0x90: {
			/**
			 * SUB A, B
			 * 1, 4
			 * Z 1 H C
			 */
			
        	sub(AF.A, BC.B);
        	
        	return 4;
        }
		
    	case 0x91: {
			/**
			 * SUB A, C
			 * 1, 4
			 * Z 1 H C
			 */
			
        	sub(AF.A, BC.C);
        	
        	return 4;
        }
    	
    	case 0x92: {
			/**
			 * SUB A, D
			 * 1, 4
			 * Z 1 H C
			 */
			
        	sub(AF.A, DE.D);
        	
        	return 4;
        }
		
    	case 0x93: {
			/**
			 * SUB A, E
			 * 1, 4
			 * Z 1 H C
			 */
			
        	sub(AF.A, DE.E);
        	
        	return 4;
        }
		
    	case 0x94: {
			/**
			 * SUB A, H
			 * 1, 4
			 * Z 1 H C
			 */
			
        	sub(AF.A, HL.H);
        	
        	return 4;
        }
    	
    	case 0x95: {
			/**
			 * SUB A, L
			 * 1, 4
			 * Z 1 H C
			 */
			
        	sub(AF.A, HL.L);
        	
        	return 4;
        }
		
    	case 0x96: {
			/**
			 * SUB A, (HL)
			 * 1, 4
			 * Z 1 H C
			 */
			
        	uint8_t u8 = mmu.fetch8(HL.get());
        	sub(AF.A, u8);
        	
        	return 4;
        }

    	case 0x97: {
			/**
			 * SUB A, A
			 * 1, 4
			 * Z 1 H C
			 */
			
        	sub(AF.A, AF.A);
        	
        	return 4;
        }
		
    	case 0x98: {
			/**
			 * SBC A, B
			 * 1, 4
			 * Z 1 H C
			 */
			
        	sbc(AF.A, BC.B, AF.getCarry());
        	
        	return 4;
        }
		
    	case 0x99: {
			/**
			 * SBC A, C
			 * 1, 4
			 * Z 1 H C
			 */
			
        	sbc(AF.A, BC.C, AF.getCarry());
        	
        	return 4;
        }
		
    	case 0x9A: {
			/**
			 * SBC A, D
			 * 1, 4
			 * Z 1 H C
			 */
			
        	sbc(AF.A, DE.D, AF.getCarry());
        	
        	return 4;
        }
		
    	case 0x9B: {
			/**
			 * SUB A, E
			 * 1, 4
			 * Z 1 H C
			 */
			
        	sbc(AF.A, DE.E, AF.getCarry());
        	
        	return 4;
        }
		
    	case 0x9C: {
			/**
			 * SUB A, H
			 * 1, 4
			 * Z 1 H C
			 */
			
        	sbc(AF.A, HL.H, AF.getCarry());
        	
        	return 4;
        }
		
    	case 0x9D: {
			/**
			 * SUB A, L
			 * 1, 4
			 * Z 1 H C
			 */
			
        	sbc(AF.A, HL.L, AF.getCarry());
        	
        	return 4;
        }
		
    	case 0x9E: {
			/**
			 * SUB A, (HL)
			 * 1, 4
			 * Z 1 H C
			 */
			
        	uint8_t u8 = mmu.fetch8(HL.get());
        	sbc(AF.A, u8, AF.getCarry());
        	
        	return 4;
        }
    	
        case 0x9F: {
            /**
             * SBC A, A
             * 1, 4
             *
             * Z N H C
             */
            
            uint8_t carry = AF.getCarry() ? 1 : 0;
            uint8_t result = AF.A - AF.A - carry;
            
            AF.setZero(result == 0);
            AF.setSubtract(true);
            AF.setHalfCarry((AF.A & 0x0F) < ((AF.A & 0x0F) + carry));
            AF.setCarry(carry > 0);
            
            AF.A = result;
            
            return 4;
        }
		
        case 0xA0: {
            /**
             * AND A, B
             * 1, 4
             *
             * Z 0 1 0
             */
            
            AF.A &= BC.B;
            
            AF.setZero(AF.A == 0);
            AF.setSubtract(false);
            AF.setHalfCarry(true);
            AF.setCarry(false);
            
            return 4;
        }

		case 0xA1: {
            /**
             * AND A, C
             * 1, 4
             *
             * Z 0 1 0
             */
            
            AF.A &= BC.C;
            
            AF.setZero(AF.A == 0);
            AF.setSubtract(false);
            AF.setHalfCarry(true);
            AF.setCarry(false);
            
            return 4;
		}

		case 0xA2: {
        	/**
			 * AND A, D
			 * 1, 4
			 *
			 * Z 0 1 0
			 */
            
        	AF.A &= DE.D;
            
        	AF.setZero(AF.A == 0);
        	AF.setSubtract(false);
        	AF.setHalfCarry(true);
        	AF.setCarry(false);
            
        	return 4;
		}

    	case 0xA3: {
        	/**
			 * AND A, E
			 * 1, 4
			 *
			 * Z 0 1 0
			 */
            
        	AF.A &= DE.E;
            
        	AF.setZero(AF.A == 0);
        	AF.setSubtract(false);
        	AF.setHalfCarry(true);
        	AF.setCarry(false);
            
        	return 4;
		}

    	case 0xA4: {
        	/**
			 * AND A, H
			 * 1, 4
			 *
			 * Z 0 1 0
			 */
            
        	AF.A &= HL.H;
            
        	AF.setZero(AF.A == 0);
        	AF.setSubtract(false);
        	AF.setHalfCarry(true);
        	AF.setCarry(false);
            
        	return 4;
		}

    	case 0xA5: {
        	/**
			 * AND A, L
			 * 1, 4
			 *
			 * Z 0 1 0
			 */
            
        	AF.A &= HL.L;
            
        	AF.setZero(AF.A == 0);
        	AF.setSubtract(false);
        	AF.setHalfCarry(true);
        	AF.setCarry(false);
            
        	return 4;
		}

    	case 0xA6: {
        	/**
			 * AND A, (HL)
			 * 1, 4
			 *
			 * Z 0 1 0
			 */
            
        	AF.A &= mmu.fetch8(HL.get());
            
        	AF.setZero(AF.A == 0);
        	AF.setSubtract(false);
        	AF.setHalfCarry(true);
        	AF.setCarry(false);
            
        	return 4;
		}
		
    	case 0xA7: {
        	/**
			 * AND A, A
			 * 1, 4
			 *
			 * Z 0 1 0
			 */

        	// Will always result in the same value,
        	// no need to do an AND operation.
        	//AF.A &= AF.A;
            
        	AF.setZero(AF.A == 0);
        	AF.setSubtract(false);
        	AF.setHalfCarry(true);
        	AF.setCarry(false);
            
        	return 4;
		}

		case 0xA8: {
	        /**
	         * XOR A, B
	         * 1, 4
	         * Z 0 0 0
	         */
			
        	AF.A ^= BC.B;
			
        	AF.setZero(AF.A == 0);
        	AF.setSubtract(false);
        	AF.setHalfCarry(false);
        	AF.setCarry(false);
        	
        	return 4;
        }
		
        case 0xA9: {
            /**
             * XOR A, C
             * 1, 4
             * Z 0 0 0
             */
            
            AF.A ^= BC.C;
            
            AF.setZero(AF.A == 0);
            AF.setSubtract(false);
            AF.setHalfCarry(false);
            AF.setCarry(false);
            
            return 4;
        }
		
    	case 0xAA: {
            /**
             * XOR A, D
             * 1, 4
             * Z 0 0 0
             */
			
            AF.A ^= DE.D;
        	
            AF.setZero(AF.A == 0);
            AF.setSubtract(false);
            AF.setHalfCarry(false);
            AF.setCarry(false);
            
            return 4;
        }
		
    	case 0xAB: {
            /**
             * XOR A, E
             * 1, 4
             * Z 0 0 0
             */
            
            AF.A ^= DE.E;
            
            AF.setZero(AF.A == 0);
            AF.setSubtract(false);
            AF.setHalfCarry(false);
            AF.setCarry(false);
            
            return 4;
        }
		
    	case 0xAC: {
            /**
             * XOR A, H
             * 1, 4
             * Z 0 0 0
             */
            
            AF.A ^= HL.H;
            
            AF.setZero(AF.A == 0);
            AF.setSubtract(false);
            AF.setHalfCarry(false);
            AF.setCarry(false);
            
            return 4;
        }
    	
    	case 0xAD: {
            /**
             * XOR A, L
             * 1, 4
             * Z 0 0 0
             */
            
            AF.A ^= HL.L;
            
            AF.setZero(AF.A == 0);
            AF.setSubtract(false);
            AF.setHalfCarry(false);
            AF.setCarry(false);
            
            return 4;
        }

    	case 0xAE: {
            /**
             * XOR A, (HL)
             * 1, 4
             * Z 0 0 0
             */
            
            AF.A ^= mmu.fetch8(HL.get());
            
            AF.setZero(AF.A == 0);
            AF.setSubtract(false);
            AF.setHalfCarry(false);
            AF.setCarry(false);
            
            return 4;
        }
		
    	case 0xAF: {
            /**
             * XOR A, A
             * 1, 4
             * Z 0 0 0
             */
			
        	// No need to
            AF.A ^= AF.A;
            
            AF.setZero(AF.A == 0);
            AF.setSubtract(false);
            AF.setHalfCarry(false);
            AF.setCarry(false);
            
            return 4;
        }
        
        case 0xB0: {
            /**
             * OR A, B
             * 1, 4
             * Z 0 0 0
             */
            
            AF.A |= BC.B;
            
            AF.setZero(AF.A == 0);
            AF.setSubtract(false);
            AF.setHalfCarry(false);
            AF.setCarry(false);
            
            return 4;
        }
		
    	case 0xB1: {
            /**
             * OR A, C
             * 1, 4
             * Z 0 0 0
             */
            
            AF.A |= BC.C;
            
            AF.setZero(AF.A == 0);
            AF.setSubtract(false);
            AF.setHalfCarry(false);
            AF.setCarry(false);
            
            return 4;
        }

    	case 0xB2: {
            /**
             * OR A, D
             * 1, 4
             * Z 0 0 0
             */
            
            AF.A |= DE.D;
        	
            AF.setZero(AF.A == 0);
            AF.setSubtract(false);
            AF.setHalfCarry(false);
            AF.setCarry(false);
            
            return 4;
        }
		
    	case 0xB3: {
            /**
             * OR A, E
             * 1, 4
             * Z 0 0 0
             */
            
            AF.A |= DE.E;
        	
            AF.setZero(AF.A == 0);
            AF.setSubtract(false);
            AF.setHalfCarry(false);
            AF.setCarry(false);
            
            return 4;
        }

    	case 0xB4: {
            /**
             * OR A, H
             * 1, 4
             * Z 0 0 0
             */
            
            AF.A |= HL.H;
        	
            AF.setZero(AF.A == 0);
            AF.setSubtract(false);
            AF.setHalfCarry(false);
            AF.setCarry(false);
            
            return 4;
        }
    	
    	case 0xB5: {
            /**
             * OR A, L
             * 1, 4
             * Z 0 0 0
             */
            
            AF.A |= HL.L;
        	
            AF.setZero(AF.A == 0);
            AF.setSubtract(false);
            AF.setHalfCarry(false);
            AF.setCarry(false);
            
            return 4;
        }
    	
    	case 0xB6: {
            /**
             * OR A, (HL)
             * 1, 4
             * Z 0 0 0
             */
            
            AF.A |= mmu.fetch8(HL.get());
        	
            AF.setZero(AF.A == 0);
            AF.setSubtract(false);
            AF.setHalfCarry(false);
            AF.setCarry(false);
            
            return 4;
        }
		
    	case 0xB7: {
            /**
             * OR A, A
             * 1, 4
             * Z 0 0 0
             */
			
        	// Would always give the same result
            //AF.A |= AF.A;
        	
            AF.setZero(AF.A == 0);
            AF.setSubtract(false);
            AF.setHalfCarry(false);
            AF.setCarry(false);
            
            return 4;
        }
    	
    	case 0xB8: {
            /**
             * CP A, B
             * 1, 4
             * Z 1 H C
             */
			
        	uint8_t result = AF.A - BC.B;
        	
            AF.setZero(result == 0);
            AF.setSubtract(true);
            AF.setHalfCarry((AF.A & 0xF) < (BC.B & 0xF));
            AF.setCarry(AF.A < BC.B);
            
            return 4;
        }

    	case 0xB9: {
	        /**
			 * CP A, C
			 * 1, 4
			 * Z 1 H C
			 */
			
        	uint8_t result = AF.A - BC.C;
        	
        	AF.setZero(result == 0);
        	AF.setSubtract(true);
        	AF.setHalfCarry((AF.A & 0xF) < (BC.C & 0xF));
        	AF.setCarry(AF.A < BC.C);
            
        	return 4;
        }

    	case 0xBA: {
	        /**
			 * CP A, D
			 * 1, 4
			 * Z 1 H C
			 */
			
        	uint8_t result = AF.A - DE.D;
        	
        	AF.setZero(result == 0);
        	AF.setSubtract(true);
        	AF.setHalfCarry((AF.A & 0xF) < (DE.D & 0xF));
        	AF.setCarry(AF.A < DE.D);
            
        	return 4;
        }

    	case 0xBB: {
	        /**
			 * CP A, E
			 * 1, 4
			 * Z 1 H C
			 */
			
        	uint8_t result = AF.A - DE.E;
        	
        	AF.setZero(result == 0);
        	AF.setSubtract(true);
        	AF.setHalfCarry((AF.A & 0xF) < (DE.E & 0xF));
        	AF.setCarry(AF.A < DE.E);
            
        	return 4;
        }
		
    	case 0xBC: {
	        /**
			 * CP A, E
			 * 1, 4
			 * Z 1 H C
			 */
			
        	uint8_t result = AF.A - HL.H;
        	
        	AF.setZero(result == 0);
        	AF.setSubtract(true);
        	AF.setHalfCarry((AF.A & 0xF) < (HL.H & 0xF));
        	AF.setCarry(AF.A < HL.H);
            
        	return 4;
        }
		
    	case 0xBD: {
	        /**
			 * CP A, L
			 * 1, 4
			 * Z 1 H C
			 */
			
        	uint8_t result = AF.A - HL.L;
        	
        	AF.setZero(result == 0);
        	AF.setSubtract(true);
        	AF.setHalfCarry((AF.A & 0xF) < (HL.L & 0xF));
        	AF.setCarry(AF.A < HL.L);
            
        	return 4;
        }
		
    	case 0xBE: {
	        /**
			 * CP A, (HL)
			 * 1, 8
			 * Z 1 H C
			 */
			
        	uint8_t val = mmu.fetch8(HL.get());
        	uint8_t result = AF.A - val;
        	
        	AF.setZero(result == 0);
        	AF.setSubtract(true);
        	AF.setHalfCarry((AF.A & 0xF) < (val & 0xF));
        	AF.setCarry(AF.A < val);
        	
        	return 8;
        }
		
    	case 0xBF: {
	        /**
			 * CP A, A
			 * 1, 4
			 * Z 1 H C
			 */
			
        	//uint8_t result = AF.A - AF.A;
        	
        	AF.setZero(true);
        	AF.setSubtract(true);
        	AF.setHalfCarry(false);
        	AF.setCarry(false);
            
        	return 4;
        }
    	
        case 0xC0: {
            /**
             * RET NZ
             * 1, (8 without branch, 20 with branch)
             */
            
            if (!AF.getZero()) {
                uint16_t address = mmu.fetch16(SP);
                SP += 2;
                
                PC = address;
                
                return 20;
            }
            
            return 8;
        }
		
        case 0xC1: {
            /**
             * POP BC
             * 1, 12
             */
            
            BC.C = mmu.fetch8(SP++);
            BC.B = mmu.fetch8(SP++);
            
            return 12;
        }

		case 0xC2: {
			/**
			 * JP NZ, u16
			 * 3, 12 (16 with branch)
			 */
			
			if(!AF.getZero()) {
				uint16_t u16 = mmu.fetch16(PC);
				
				PC = u16;
				
				return 16;
			}
        	
        	PC += 2;
			
        	return 12;
        }
		
        case 0xC3: {
            /**
             * JP u16
             * 3, 16
             */
            
            uint16_t address = mmu.fetch16(PC);
            PC = address;
        	
            return 16;
        }
		
		case 0xC4: {
			/**
			 * CALL NZ, u16
			 * 3, 12 (24 with branch)
			 * - - - -
			 */
			
        	if(!AF.getZero()) {
				pushToStack(PC + 2);
        		
        		uint16_t u16 = mmu.fetch16(PC);
        		PC = u16;
        		
        		return 24;
        	}
			
        	PC += 2;
        	
        	return 12;
        }
		
		case 0xC5: {
			/**
			 * PUSH BC
			 * 1, 16			 * - - - -
			 */
			
        	pushToStack(BC.get());
			
        	return 16;
        }
		
		case 0xC6: {
			/**
			 * ADD A, u8
			 * 2, 8
			 * A 0 H C
			 */
			
        	uint8_t u8 = mmu.fetch8(PC++);
        	add(AF.A, u8);
			
        	/*
        	uint16_t result = AF.A + u8;
			
        	AF.A = static_cast<uint8_t>(result);
			
        	AF.setZero(AF.A == 0);
        	AF.setSubtract(false);
        	AF.setHalfCarry((AF.A & 0xF) + (u8 & 0xF4) > 0xF);
        	AF.setCarry(result > 0xFF);*/
        	
        	return 8;
        }
		
		case 0xC8: {
			/**
			 * RET Z
			 * 1, 8 (20 with branch)
			 * - - - -
			 */
			
        	if(AF.getZero()) {
        		uint16_t address = mmu.fetch16(SP);
        		SP += 2;
				
        		PC = address;
				
        		return 20;
        	}
        	
        	return 8;
        }
    	
        case 0xC9: {
            /*
             * RET
             * 1, 16
             */
            
            uint16_t d16 = mmu.fetch16(SP);
        	
            PC = d16;
            SP += 2;
            
            return 16;
        }
		
		case 0xCB: {
			/**
			 * PREFIX CB
			 * 1, 4
			 */
			
			return decodePrefix(fetchOpCode());
        }
		
        case 0xCD: {
            /**
             * CALL u16
             * 3, 24
             */
            
            pushToStack(PC + 2);
            
            uint16_t u16 = mmu.fetch16(PC);
            PC = u16;
            
            return 24;
        }
		
		case 0xCE: {
			/**
			 * ADC A, u8
			 * 2, 8
			 * Z 0 H C
			 */

        	uint8_t u8 = mmu.fetch8(PC++);
        	
        	adc(AF.A, u8, AF.getCarry());
        	
        	return 8;
        }
		
		case 0xD0: {
			/**
			 * RET NC
			 * 1, 8 (20 with branch)
			 * - - - -
			 */
			
        	if(!AF.getCarry()) {
				uint16_t address = static_cast<uint16_t>(mmu.fetch8(SP + 1) << 8) | static_cast<uint16_t>(mmu.fetch8(SP));
				
        		PC = address;
        		
        		SP += 2;
        		
        		return 20;
        	}
        	
        	return 8;
        }
		
		case 0xD1: {
			/**
			 * POP DE
			 * 1, 12
			 * - - - -
			 */
			
        	popReg(DE.D, DE.E);
        	
        	return 12;
        }
    	
		case 0xD5: {
			/**
			 * PUSH DE
			 * 1, 16
			 * - - - -
			 */
        	
        	pushToStack(DE.get());
        	
        	return 16;
        }
		
		case 0xD6: {
			/**
			 * SUB A, u8
			 * 2, 8
			 * Z 1 H C
			 */
			
        	uint8_t u8 = mmu.fetch8(PC++);
        	sub(AF.A, u8);
        	
        	/*uint8_t result = AF.A - u8;
			
        	AF.setZero(result == 0);
        	AF.setSubtract(true);
        	AF.setHalfCarry((AF.A & 0xF) < (u8 < 0xF));
        	AF.setCarry(AF.A < u8);
			
        	AF.A = result;*/
        	
        	return 8;
        }

		case 0xD8: {
			/**
			 * RET C
			 * 1, 8 (20 with branch)
			 * - - - -
			 */
			
        	if(AF.getCarry()) {
        		PC = mmu.fetch16(SP);
        		SP += 2;
				
        		return 20;
        	}
			
        	return 8;
        }
        
        case 0xDF: {
            // RST 18H
            // 1, 16
            
            rst(0x18);
            
            return 16;
        }
        
        case 0xE0: {
            /*
             * LD (FF00 + U8), A
             * 2, 12
             */
			
        	uint8_t u8 = mmu.fetch8(PC++);
        	uint16_t address = 0xFF00 | u8;
        	/*printf("%x %x %x\n", u8, address, AF.A);
        	std::cerr << "";*/
        	
            mmu.write8(address, AF.A);
            
            return 12;
        }
		
        case 0xE1: {
            /**
             * POP HL,
             * 1, 12
             */
            
            uint16_t val = mmu.fetch16(SP);
            HL = val;
            
            SP += 2;
            
            return 12;
        }
        
        case 0xE5: {
            /**
             * PUSH HL
             * 1, 16
             */
            
            pushToStack(HL.get());
            
            return 16;
        }
		
		case 0xE6: {
			/**
			 * AND A, 8u
			 * 2, 8
			 * Z 0 1 0
			 */
			
        	AF.A &= mmu.fetch8(PC++);
        	
        	AF.setZero(AF.A == 0);
        	AF.setSubtract(false);
        	AF.setHalfCarry(true);
        	AF.setCarry(false);
        	
        	return 8;
        }
		
		case 0xE8: {
			/**
			 * ADD SP, i8
			 * 2, 16
			 * 0 0 H C
			 */
			
        	int8_t i8 = static_cast<int8_t>(mmu.fetch8(PC++));
        	uint16_t res = SP + i8;
			
        	AF.setHalfCarry((SP & 0xF) + (i8 & 0xF) > 0xF);
        	AF.setCarry(res > 0xFF);
			
        	SP = res;
        	
        	return 16;
        }
		
        case 0xE9: {
            /**
             * JP HL
             * 1, 4
             */
            
            PC = HL.get();
            
            return 4;
        }
        
        case 0xEA: {
            /**
             * LD (d16), A
             * 3, 16
             */
            
            mmu.write8(mmu.fetch16(PC), AF.A);
            
            PC += 2;
            
            return 16;
        }
		
		case 0xEE: {
			/**
			 * XOR A, u8
			 * 2, 8
			 * Z 0 0 0
			 */

        	uint8_t u8 = mmu.fetch8(PC++);
        	xor8(AF.A, u8);
        	
        	return 8;
        }
		
		case 0xF0: {
			/**
			 * LD A,(FF00 + u8)
			 * 2, 12
			 * - - - -
			 */
			
        	uint8_t u8 = mmu.fetch8(PC++);
        	AF.A = mmu.fetch8(0xFF00 | u8);
        	
        	return 12;
        }
        
        case 0xF1: {
            /**
             * POP AF
             * 1, 12
             */
            
            AF.F = mmu.fetch8(SP++);
            AF.A = mmu.fetch8(SP++);
            
            return 12;
        }
        
        case 0xF3: {
            /**
             * DI - Disable Interrupts by clearing the IME flag
             *
             * 1, 4
             */
            
            interruptHandler.IME = false;
            
            return 4;
        }
		
        case 0xF5: {
            /**
             * PUSH AF
             * 1, 16
             */
            
            pushToStack(AF.get());
            
            return 16;
        }
		
		case 0xF9: {
			/**
			 * LD SP, HL
			 * 1, 8
			 * - - - -
			 */
			
        	SP = HL.get();
        	
        	return 8;
        }
		
        case 0xFA: {
            /**
             * LD A, (u16)
             * 3, 16
             */
            
            uint16_t u16 = mmu.fetch16(PC);
            AF.A  = mmu.fetch8(u16);
            
            PC += 2;
            
            return 16;
        }
        
        case 0xFB: {
            /**
             * EI - Enable Interrupts
             * 1, 4
             */
            
            interruptHandler.IME = true;
            
            return 4;
        }
		
		case 0xFE: {
			/**
			 * CP A, u8
			 * 2, 8
			 *
			 * Z 1 H C
			 */
			
        	uint8_t u8 = mmu.fetch8(PC++);
			uint8_t A = AF.A;
			uint8_t result = A - u8;
        	
			//AF.A = A;
			
        	AF.setZero(result == 0);
        	AF.setSubtract(true);
        	AF.setHalfCarry((A & 0xF) < (u8 & 0xF));
        	AF.setCarry(A < u8);
			
        	return 8;
        }
		
    	case 0xFF: {
            // RST 38H
            // Push the current PC onto the stack
            // 1, 16
            
            rst(0x38);
            
            return 16;
        }
    	
        // ld reg, reg
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x47: // ld b,reg
        case 0x48:
        case 0x49:
        case 0x4a:
        case 0x4b:
        case 0x4c:
        case 0x4d:
        case 0x4f: // ld,reg
        case 0x50:
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x57: // ld d,reg
        case 0x58:
        case 0x59:
        case 0x5a:
        case 0x5b:
        case 0x5c:
        case 0x5d:
        case 0x5f: // ld e,reg
        case 0x60:
        case 0x61:
        case 0x62:
        case 0x63:
        case 0x64:
        case 0x65:
        case 0x67: // ld h,reg
        case 0x68:
        case 0x69:
        case 0x6a:
        case 0x6b:
        case 0x6c:
        case 0x6d:
        case 0x6f: // ld l,reg
        case 0x78:
        case 0x79:
        case 0x7a:
        case 0x7b:
        case 0x7c:
        case 0x7d:
        case 0x7f: {
        	// https://www.reddit.com/r/EmuDev/comments/7ljc41/how_to_algorithmically_parse_gameboy_opcodes/
			
        	/*if(PC == 0x0201) {
        		printf("LD B A");
        	}*/
        	
        	/**
        	 * Had an issue here,
        	 * in 'getRegister' I swapped A and F
        	 */
        	
            // ld a,reg
            uint8_t& dst = getRegister(opcode >> 3 & 7);
            uint8_t src = getRegister(opcode & 7);
            
            dst = src;
            
            return 4;
        }
        
        case 0x80:
        case 0x81:
        case 0x82:
        case 0x83:
        case 0x84:
        case 0x85:
        case 0x87: // add a reg
        case 0x88:
        case 0x89:
        case 0x8a:
        case 0x8b:
        case 0x8c:
        case 0x8d:
        case 0x8f: {
            // adc a,reg
            bool carry = (opcode & 8) && AF.getCarry();
            
            adc(AF.A, getRegister(opcode & 7), carry);
            
            return 4;
        }
        
        default: {
            printf("Unknown instruction; %x\n", opcode);
            std::cerr << "";
            
            return 4;
        }
    }
 }

int CPU::decodePrefix(uint16_t opcode) {
	switch (opcode) {
		case 0x00: {
			/**
			 * RLC B
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rlc(BC.B);
			
			return 8;
		}
		
		case 0x01: {
			/**
			 * RLC C
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rlc(BC.C);
			
			return 8;
		}
		
		case 0x02: {
			/**
			 * RLC D
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rlc(DE.D);
			
			return 8;
		}
		
		case 0x03: {
			/**
			 * RLC E
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rlc(DE.E);
			
			return 8;
		}
		
		case 0x04: {
			/**
			 * RLC H
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rlc(HL.H);
			
			return 8;
		}
		
		case 0x05: {
			/**
			 * RLC H
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rlc(HL.L);
			
			return 8;
		}
		
		case 0x06: {
			/**
			 * RLC (HL)
			 * 2, 8
			 * Z 0 0 C
			 */
			
			uint8_t u8 = mmu.fetch8(HL.get());
			rlc(u8);
			
			return 8;
		}
		
		case 0x07: {
			/**
			 * RLC A
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rlc(AF.A);
			
			return 8;
		}

		case 0x08: {
			/**
			 * RCC B
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rrc(BC.B);
			
			return 8;
		}
		
		case 0x09: {
			/**
			 * RCC C
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rrc(BC.C);
			
			return 8;
		}
		
		case 0x0A: {
			/**
			 * RCC D
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rrc(DE.D);
			
			return 8;
		}
		
		case 0x0B: {
			/**
			 * RCC E
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rrc(DE.E);
			
			return 8;
		}
		
		case 0x0C: {
			/**
			 * RCC H
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rrc(HL.H);
			
			return 8;
		}
		
		case 0x0D: {
			/**
			 * RCC l
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rrc(HL.L);
			
			return 8;
		}
		
		case 0x0E: {
			/**
			 * RCC (HL)
			 * 2, 16
			 * Z 0 0 C
			 */

			uint8_t u8 = mmu.fetch8(HL.get());
			
			// Save the least significant bit (bit 0)
			uint8_t bit0 = u8 & 0x01;
			
			// Rotate the value right
			u8 = (u8 >> 1) | (bit0 << 7);
			
			mmu.write8(HL.get(), u8);
			
			AF.setZero(u8 == 0);
			AF.setSubtract(false);
			AF.setHalfCarry(false);
			AF.setCarry(bit0 == 1);
			
			return 16;
		}
		
		case 0x0F: {
			/**
			 * RCC A
			 * 2, 16
			 * Z 0 0 C
			 */
			
			rrc(AF.A);
			
			return 8;
		}
		
		case 0x10: {
			/**
			 * RL B
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rl(BC.B);
			
			return 8;
		}
		
		case 0x11: {
			/**
			 * RL C
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rl(BC.C);
			
			return 8;
		}
		
		case 0x12: {
			/**
			 * RL D
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rl(DE.D);
			
			return 8;
		}
		
		case 0x13: {
			/**
			 * RL E
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rl(DE.E);
			
			return 8;
		}
		
		case 0x14: {
			/**
			 * RL H
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rl(HL.H);
			
			return 8;
		}
		
		case 0x15: {
			/**
			 * RL L
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rl(HL.L);
			
			return 8;
		}
		
		case 0x16: {
			/**
			 * RL (HL)
			 * 2, 16
			 * Z 0 0 C
			 */
			
			uint8_t value = mmu.fetch8(HL.get());
			uint8_t bit7 = (value >> 7) & 0x01;
			
			value = (value << 1) | (AF.getCarry() ? 1 : 0);
			
			mmu.write8(HL.get(), value);
			
			AF.setZero(value == 0);
			AF.setSubtract(false);
			AF.setHalfCarry(false);
			AF.setCarry(bit7 == 1);
			
			return 16;
		}
		
		case 0x17: {
			/**
			 * RL A
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rl(AF.A);
			
			return 8;
		}
		
		case 0x18: {
			/**
			 * RR B
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rr(BC.B);
			
			return 8;
		}
		
		case 0x19: {
			/**
			 * RR C
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rr(BC.C);
			
			return 8;
		}
		
		case 0x1A: {
			/**
			 * RR D
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rr(DE.D);
			
			return 8;
		}
		
		case 0x1B: {
			/**
			 * RR E
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rr(DE.E);
			
			return 8;
		}
		
		case 0x1C: {
			/**
			 * RR H
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rr(HL.H);
			
			return 8;
		}
		
		case 0x1D: {
			/**
			 * RR L
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rr(HL.L);
			
			return 8;
		}
		
		case 0x1E: {
			/**
			 * RR (HL)
			 * 2, 16
			 * Z 0 0 C
			 */
			
			uint8_t value = mmu.fetch8(HL.get());
			uint8_t bit0 = value & 0x01;
			
			value = (value >> 1) | (AF.getCarry() ? 0x80 : 0x00);
			
			mmu.write8(HL.get(), value);
			
			AF.setZero(value == 0);
			AF.setSubtract(false);
			AF.setHalfCarry(false);
			AF.setCarry(bit0 == 1);
			
			return 16;
		}
		
		case 0x1F: {
			/**
			 * RR A
			 * 2, 8
			 * Z 0 0 C
			 */
			
			rr(AF.A);
			
			return 8;
		}
		
		case 0x20: {
			/**
			 * SLA B
			 * 2, 8
			 * Z 0 0 C
			 */
			
			sla(BC.B);
			
			return 8;
		}

		case 0x21: {
			/**
			 * SLA C
			 * 2, 8
			 * Z 0 0 C
			 */
			
			sla(BC.C);
			
			return 8;
		}
		
		case 0x22: {
			/**
			 * SLA D
			 * 2, 8
			 * Z 0 0 C
			 */
			
			sla(DE.D);
			
			return 8;
		}
		
		case 0x23: {
			/**
			 * SLA E
			 * 2, 8
			 * Z 0 0 C
			 */
			
			sla(DE.E);
			
			return 8;
		}

		case 0x24: {
			/**
			 * SLA H
			 * 2, 8
			 * Z 0 0 C
			 */
			
			sla(HL.H);
			
			return 8;
		}
		
		case 0x25: {
			/**
			 * SLA L
			 * 2, 8
			 * Z 0 0 C
			 */
			
			sla(HL.L);
			
			return 8;
		}
		
		case 0x26: {
			/**
			 * SLA (HL)
			 * 2, 16
			 * Z 0 0 C
			 */
			
			uint8_t value = mmu.fetch8(HL.get());
			
			// Perform the shift left operation
			uint8_t result = value << 1;
			
			// Determine the carry flag (bit 7 of the original value)
			bool carryOut = (value & 0x80) != 0;
			
			AF.setZero(result == 0);
			AF.setSubtract(false);
			AF.setHalfCarry(false);
			AF.setCarry(carryOut);
			
			return 16;
		}
		
		case 0x27: {
			/**
			 * SLA A
			 * 2, 8
			 * Z 0 0 C
			 */
			
			sla(AF.A);
			
			return 8;
		}
		
		case 0x28: {
			/**
			 * SRA B
			 * 2, 8
			 * Z 0 0 C
			 */
			
			sra(BC.B);
			
			return 8;
		}
		
		case 0x29: {
			/**
			 * SRA C
			 * 2, 8
			 * Z 0 0 C
			 */
			
			sra(BC.C);
			
			return 8;
		}
		
		case 0x2A: {
			/**
			 * SRA D
			 * 2, 8
			 * Z 0 0 C
			 */
			
			sra(DE.D);
			
			return 8;
		}
		
		case 0x2B: {
			/**
			 * SRA E
			 * 2, 8
			 * Z 0 0 C
			 */
			
			sra(DE.E);
			
			return 8;
		}
		
		case 0x2C: {
			/**
			 * SRA H
			 * 2, 8
			 * Z 0 0 C
			 */
			
			sra(HL.H);
			
			return 8;
		}
		
		case 0x2D: {
			/**
			 * SRA L
			 * 2, 8
			 * Z 0 0 C
			 */
			
			sra(HL.L);
			
			return 8;
		}
		
		case 0x2E: {
			/**
			 * SRA (HL)
			 * 2, 16
			 * Z 0 0 C
			 */
			
			uint16_t address = HL.get();
			uint8_t value = mmu.fetch8(address);
			
			// Preserve the sign bit (bit 7) and shift right
			uint8_t result = (value >> 1) | (value & 0x80);  // Keep bit 7
			
			bool carryOut = (value & 0x01) != 0;
			
			AF.setZero(result == 0);
			AF.setSubtract(false);
			AF.setHalfCarry(false);
			AF.setCarry(carryOut);
			
			mmu.write8(address, result);
			
			return 16;
		}
		
		case 0x2F: {
			/**
			 * SRA A
			 * 2, 8
			 * Z 0 0 C
			 */
			
			sra(AF.A);
			
			return 8;
		}
		
		case 0x30: {
		    /**
		     * SWAP B
		     * 2, 8
		     * Z 0 0 0
		     */
			
		    swap(BC.B);
			
		    return 8;
		}
		
		case 0x31: {
		    /**
		     * SWAP C
		     * 2, 8
		     * Z 0 0 0
		     */
			
		    swap(BC.C);
			
		    return 8;
		}
		
		case 0x32: {
		    /**
		     * SWAP D
		     * 2, 8
		     * Z 0 0 0
		     */
			
		    swap(DE.D);
			
		    return 8;
		}
		
		case 0x33: {
		    /**
		     * SWAP E
		     * 2, 8
		     * Z 0 0 0
		     */
			
		    swap(DE.E);
			
		    return 8;
		}
		
		case 0x34: {
		    /**
		     * SWAP H
		     * 2, 8
		     * Z 0 0 0
		     */
			
		    swap(HL.H);
			
		    return 8;
		}
		
		case 0x35: {
		    /**
		     * SWAP L
		     * 2, 8
		     * Z 0 0 0
		     */
			
		    swap(HL.L);
			
		    return 8;
		}
		
		case 0x36: {
		    /**
		     * SWAP (HL)
		     * 2, 16
		     * Z 0 0 0
		     */
			
			// Idk why I didn't do it like this before..
		    uint8_t value = mmu.fetch8(HL.get());
		    swap(value);
		    mmu.write8(HL.get(), value);
			
		    return 16;
		}
		
		case 0x37: {
			/**
			 * SWAP A
			 * 2, 8
			 * Z 0 0 0
			 */
			
			swap(AF.A);
			
			return 8;
		}
		
		case 0x38: {
			/**
			 * SRL B
			 * 2, 8
			 * Z 0 0 C
			 */
			
			srl(BC.B);
			
			return 8;
		}
		
		case 0x39: {
			/**
			 * SRL c
			 * 2, 8
			 * Z 0 0 C
			 */
			
			srl(BC.C);
			
			return 8;
		}
		
		case 0x3A: {
			/**
			 * SRL D
			 * 2, 8
			 * Z 0 0 C
			 */
			
			srl(DE.D);
			
			return 8;
		}
		
		case 0x3B: {
			/**
			 * SRL E
			 * 2, 8
			 * Z 0 0 C
			 */
			
			srl(DE.E);
			
			return 8;
		}
		
		case 0x3C: {
			/**
			 * SRL H
			 * 2, 8
			 * Z 0 0 C
			 */
			
			srl(HL.H);
			
			return 8;
		}
		
		case 0x3D: {
			/**
			 * SRL L
			 * 2, 8
			 * Z 0 0 C
			 */
			
			srl(HL.L);
			
			return 8;
		}
		
		case 0x3E: {
			/**
			 * SRL (HL)
			 * 2, 8
			 * Z 0 0 C
			 */
			
			uint16_t address = HL.get();
			uint8_t value = mmu.fetch8(address);
			
			bool carryOut = (value & 0x01) != 0;  // Check if the LSB is 1, which will be the new carry
			
			value >>= 1;  // Perform logical shift right
			
			AF.setZero(value == 0);
			AF.setSubtract(false);
			AF.setHalfCarry(false);
			AF.setCarry(carryOut);
			
			mmu.write8(address, value);
			
			return 8;
		}
		
		case 0x3F: {
			/**
			 * SRL A
			 * 2, 8
			 * Z 0 0 C
			 */
			
			srl(AF.A);
			
			return 8;
		}
		
		case 0x7E: {
			/**
			 * BIT 7, (HL)
			 * 2, 12
			 * Z 0 1 -
			 */
			
			uint16_t v = HL.get();
			checkBit(7, v);
			
			PC++;
			
			return 12;
		}
		
		case 0xBE: {
			/**
			 * REST 7, (HL)
			 * 2, 16
			 * - - - -
			 */
			
			uint16_t hl = HL.get();
			clearBit(7, hl);
			
			PC++;
			
			return 16;
		}
		
		case 0xCE: {
			/**
			 * SET 1, (HL)
			 * 2, 16
			 * - - - -
			 */
			
			uint16_t hl = HL.get();
			setBit(1, hl);
			
			return 16;
		}
		
		case 0xE6: {
			/**
			 * SET 4, (HL)
			 * 2, 16
			 * - - - -
			 */
			
			uint16_t hl = HL.get();
			setBit(2, hl);
			
			return 16;
		}
		
		case 0xE9: {
			/**
			 * SET 5, C
			 * 2, 8
			 * - - - -
			 */
			
			setBit(5, BC.C);
			
			return 8;
		}
		
		case 0xF6: {
			/**
			 * SET 6, (HL)
			 * 2, 16
			 * - - - -
			 */
			
			uint16_t hl = HL.get();
			setBit(6, hl);
			
			return 16;
		}
		
		default: {
			printf("Unknown instruction; %x\n", opcode);
			std::cerr << "";
            
			return 4;
		}
	}
}

void CPU::popReg(uint8_t& reg) {
	reg = mmu.fetch8(SP++);
}

void CPU::popReg(uint8_t& high, uint8_t& low) {
	low = mmu.fetch8(SP++);
	high = mmu.fetch8(SP++);
}

void CPU::jr(int8_t& offset) {
	PC += offset + 1; // For the byte
}

int CPU::jrc(int8_t& offset) {
	if(AF.getCarry()) {
		PC += offset + 1; // For the byte
		
		return 12;
	}
	
	PC++;

	return 8;
}

int CPU::jrnc(int8_t& offset) {
	if(!AF.getCarry()) {
		PC += offset + 1; // For the byte
		
		return 12;
	}
	
	PC++;
	
	return 8;
}

int CPU::jrz(int8_t& offset) {
	if (AF.getZero()) {
		PC += offset + 1; // For the byte
		return 12;
	}
	
	PC++;
    
	return 8;
}

int CPU::jrnz(int8_t& offset) {
	if (!AF.getZero()) {
		PC += offset + 1; // For the v
		
		return 12;
	}
	
	PC++;
	
	return 8;
}

void CPU::ld(uint8_t& regA, uint8_t& regB) {
	regA = regB;
}

void CPU::rra() {
	bool carryIn = AF.getCarry();
    
	// Save the bit that will be shifted out (the least significant bit)
	bool bitOut = (AF.A & 0x01) != 0;
    
	// Rotate the accumulator value
	AF.A = (AF.A >> 1) | (carryIn ? 0x80 : 0x00); // Place the carry bit into the MSB
    
	AF.setZero(false);
	AF.setSubtract(false);
	AF.setHalfCarry(false);
	AF.setCarry(bitOut);
}


void CPU::rst(uint16_t pc) {
    uint8_t high = (PC >> 8) & 0xFF;
    uint8_t low = PC & 0xFF;
    
    SP--;
    mmu.write8(SP, high);
    
    SP--;
    mmu.write8(SP, low);
    
    PC = pc;
}

void CPU::adc(uint8_t& regA, const uint8_t& value, bool carry) {
    uint8_t carryValue = carry ? 1 : 0;
    uint16_t result = regA + value + carryValue;
    
    AF.setZero((result & 0xFF) == 0);
    AF.setSubtract(false);
    AF.setHalfCarry(((regA & 0xF) + (value & 0xF) + carryValue) > 0xF);
    AF.setCarry(result > 0xFF);
    
    regA = result & 0xFF;
}

void CPU::add(uint8_t& regA, uint8_t& regB) {
	uint8_t u8 = regB;
	uint16_t result = regA + u8;
	
	AF.setZero((result & 0xFF) == 0);
	AF.setSubtract(false);
	AF.setHalfCarry((regA & 0xF) + (u8 & 0xF) > 0xF);
	AF.setCarry(result > 0xFF);
	
	regA = static_cast<uint8_t>(result);
}

void CPU::sbc(uint8_t& regA, const uint8_t& regB, bool carry) {
	uint8_t carryValue = carry ? 1 : 0;
	uint16_t result = regA - regB - carryValue;
	
	AF.setZero((result & 0xFF) == 0);
	AF.setSubtract(true);
	AF.setHalfCarry(((regA & 0x0F) < (regB & 0x0F) + carryValue));
	AF.setCarry(result > 0xFF);
	
	regA = result & 0xFF;
}

void CPU::sub(uint8_t& regA, uint8_t& regB) {
	uint8_t u8 = regB;
	uint8_t result = regA - u8;
	
	AF.setZero(result == 0);
	AF.setSubtract(true);
	AF.setHalfCarry((regA & 0xF) < (u8 & 0xF));
	AF.setCarry(regA < u8);
	
	regA = result;
}

void CPU::inc(uint8_t& reg) {
	uint8_t result = reg + 1;
	
	AF.setZero((result == 0));
	AF.setSubtract(false);
	AF.setHalfCarry(((reg & 0x0F) + 1) > 0x0F);
	
	reg = result;
}

void CPU::dec(uint8_t& reg) {
	reg -= 1;
	
	AF.setZero(reg == 0);
	AF.setSubtract(true);
	AF.setHalfCarry(((reg + 1) & 0x0F) == 0x00);
}

void CPU::xor8(uint8_t& regA, uint8_t& regB) {
	regA ^= regB;
	
	AF.setZero(regA == 0);
	AF.setSubtract(false);
	AF.setHalfCarry(false);
	AF.setCarry(false);
}

void CPU::checkBit(uint8_t bit, uint16_t& reg) {
	uint8_t value = mmu.fetch8(reg);
	
	bool isBit = check_bit(value, bit);
	
	AF.setZero(!isBit);
	AF.setSubtract(false);
	AF.setHalfCarry(true);
}

void CPU::clearBit(uint8_t bit, uint16_t& reg) {
	uint8_t value = mmu.fetch8(reg);

	// To clear the bit
	value &= ~(1 << bit);
	
	mmu.write8(reg, value);
}

void CPU::setBit(uint8_t bit, uint16_t& reg) {
	uint8_t value = mmu.fetch8(reg);
	
	value |= (1 << bit);
	
	mmu.write8(reg, value);
}

void CPU::setBit(uint8_t bit, uint8_t& reg) {
	reg |= (1 << bit);
}

void CPU::swap(uint8_t& reg) noexcept {
	uint8_t orig = reg;
	reg = (orig << 4) | (orig >> 4);
	
	AF.setZero(reg == 0);
	AF.setSubtract(false);
	AF.setHalfCarry(false);
	AF.setCarry(false);
}

void CPU::rlc(uint8_t& reg) {
	// Save the most significant bit (bit 7)
	uint8_t bit7 = (reg & 0x80) >> 7;
	
	// Rotate the register left
	reg = (reg << 1) | bit7;
	
	AF.setZero(reg == 0);
	AF.setSubtract(false);
	AF.setHalfCarry(false);
	AF.setCarry(bit7 == 1);
}

void CPU::rl(uint8_t& reg) {
	// Save the most significant bit (bit 7)
	uint8_t bit7 = (reg >> 7) & 0x01;
	
	// Shift the register left by one, and add the carry flag into bit 0
	reg = (reg << 1) | (AF.getCarry() ? 1 : 0);
	
	AF.setZero(reg == 0);
	AF.setSubtract(false);
	AF.setHalfCarry(false);
	AF.setCarry(bit7 == 1);
}

void CPU::rrc(uint8_t& reg) {
	// Save the least significant bit (bit 0)
	uint8_t bit0 = reg & 0x01;
	
	// Rotate the register right
	reg = (reg >> 1) | (bit0 << 7);
	
	AF.setZero(reg == 0);
	AF.setSubtract(false);
	AF.setHalfCarry(false);
	AF.setCarry(bit0 == 1);
}

void CPU::rr(uint8_t& reg) {
	bool carryIn = AF.getCarry();
    
	// Save the bit that will be shifted out (the least significant bit)
	bool bitOut = (reg & 0x01) != 0;
    
	// Rotate the register value
	reg = (reg >> 1) | (carryIn ? 0x80 : 0x00); // Place the carry bit into the MSB
    
	AF.setZero(reg == 0);
	AF.setSubtract(false);
	AF.setHalfCarry(false);
	AF.setCarry(bitOut);
}

void CPU::srl(uint8_t& reg) {
	bool carryOut = (reg & 0x01) != 0;  // Check if the LSB is 1, which will be the new carry
	
	reg >>= 1;  // Perform logical shift right
	
	AF.setZero(reg == 0);
	AF.setSubtract(false);
	AF.setHalfCarry(false);
	AF.setCarry(carryOut);
}

void CPU::sla(uint8_t& reg) {
	uint8_t value = reg;
	
	// Calculate the result of the shift
	uint8_t result = value << 1;
	
	AF.setZero(result == 0);
	AF.setSubtract(false);
	AF.setHalfCarry(false);
	AF.setCarry((value & 0x80) != 0);
	
	reg = result;
}

void CPU::sra(uint8_t& reg) {
	// Preserve the sign bit (bit 7) and shift right
	uint8_t result = (reg >> 1) | (reg & 0x80);  // Keep bit 7
	
	// Determine the carry flag (bit 0 of the original value)
	bool carryOut = (reg & 0x01) != 0;
	
	AF.setZero(result == 0);
	AF.setSubtract(false);
	AF.setHalfCarry(false);
	AF.setCarry(carryOut);
	
	reg = result;
}

void CPU::pushToStack(uint16_t value) {
	/*mmu.write8(--SP, value & 0xFF);
    mmu.write8(--SP, (value >> 8) /*should I & 0xFF?#1#);
    */
	
	SP -= 2;
    mmu.write16(SP, value);
}

uint16_t CPU::popStack() {
	uint8_t low = mmu.fetch8(SP++);
	uint8_t high = mmu.fetch8(SP++);
	
	return static_cast<uint8_t>(high << 8) | low;
}

void CPU::reset() {
    AF.A = 0x01;
    AF.F = 0xB0;
    BC.B = 0x00;
    BC.C = 0x13;
    DE.D = 0x00;
    DE.E = 0xD8;
    HL.H = 0x01;
    HL.L = 0x4D;
    
    PC = 0x0100;
    SP = 0xFFFE;
	
    mmu.clear();
}

void CPU::testCases() {
     // Helper to reset CPU state
    auto resetCPU = [&]() {
        reset();
        PC = 0x1000; // Set the initial program counter to a test address
        mmu.clear();     // Clear memory
    };
    
    // Helper to check register values
    auto checkRegisters = [&](uint8_t expectedA, uint8_t expectedF, uint8_t expectedB, uint8_t expectedC, uint8_t expectedD, uint8_t expectedE, uint8_t expectedH, uint8_t expectedL, uint16_t expectedHL, uint16_t expectedSP) {
        assert(AF.A == expectedA);
        assert(AF.F == expectedF);
        assert(BC.B == expectedB);
        assert(BC.C == expectedC);
        assert(DE.D == expectedD);
        assert(DE.E == expectedE);
        assert(HL.H == expectedH);
        assert(HL.L == expectedL);
        assert(SP == expectedSP);
    };
	
	
}
