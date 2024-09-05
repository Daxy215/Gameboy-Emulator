#include "CPU.h"

#include <cassert>

#include "InterrupHandler.h"

#include "../Memory/MMU.h"

CPU::CPU(InterruptHandler& interruptHandler, MMU& mmu)
    : interruptHandler(interruptHandler), mmu(mmu) {
    // https://gbdev.io/pandocs/Power_Up_Sequence.html#cpu-registers
    
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
             */
            
            BC += 1;
            
            return 8;
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
		
		case 0x0D: {
			/**
			 * DEC C
			 * 1, 4
			 * Z 1 H -
			 */
			
        	BC.C -= 1;
        	
        	AF.setZero(BC.C == 0);
        	AF.setSubtract(true);
        	AF.setHalfCarry(((BC.C + 1) & 0x0F) == 0x00);
			
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
        
        case 0x11: {
            /**
             * LD DE, d16
             * 3, 12
             */
            
            DE = mmu.fetch16(PC);
            
            PC += 2;
            
            return 12;
        }
		
        case 0x3C: {
            /**
             * INC A
             * 1, 4
             * Z 0 H -
             */
            
            uint8_t result = AF.A + 1;
            
            // Set the Zero flag if the result is zero
            AF.setZero((result == 0));
            
            // Reset the Subtract Flag (N)
            AF.setSubtract(false);
            
            // Set the Half Carry flag (H) if there was a carry from bit 3 to bit 4
            AF.setHalfCarry(((AF.A & 0x0F) + 1) > 0x0F);
            
            // Store the results into the A register
            AF.A = result;
            
            return 4;
        }
		
        case 0x12: {
            /**
             * LD (DE), A
             * 1, 8
             */
            
            mmu.write8(mmu.fetch16(DE.get()), AF.A);
            
            return 8;
        }
		
		case 0x14: {
			/**
			 * INC D
			 * 1, 4
			 * Z 0 H -
			 */
			
        	DE.D += 1;
        	
        	AF.setZero(DE.D == 0);
        	AF.setSubtract(false);
        	AF.setHalfCarry(((DE.D - 1) & 0x0F) == 0x0F);
			
        	return 4;
        }
    	
        case 0x18: {
            /**
             * JR i8
             * 2, 12
             */
            
            int8_t i8 = static_cast<int8_t>(mmu.fetch8(PC++));
            
            PC += i8;
            
            return 12;
        }
        
        case 0x1C: {
            /**
             * INC E
             * 1, 4
             * Z N H -
             */
			
        	DE.E += 1;
        	
            // Set the Zero flag if the result is zero
            AF.setZero(DE.E == 0);
            
            // Reset the Subtract Flag (N)
            AF.setSubtract(false);
            
            // Set the Half Carry flag (H) if there was a carry from bit 3 to bit 4
            AF.setHalfCarry(((DE.E - 1) & 0x0F) == 0x0F);
        	
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
            
            if (!AF.getZero()) {
                int8_t offset = mmu.fetch8(PC++);
                
                PC += offset;
                
                return 12;
            }
            
            PC++; // To skip the offset byte
            
            return 8;
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

        case 0x23: {
            /**
             * INC HL
             * 1, 8
             */
            
            HL = (HL + 1);
            
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
        
        case 0x39: {
            /**
             * ADD HL, SP
             * 1, 8
             * - 0 H C
             */
            
        	// Extract HL and SP values as 16-bit integers
        	uint16_t HL_value = HL.get(); // Assuming HL.get() returns the 16-bit value
        	uint16_t SP_value = SP;
			
        	// Perform the addition
        	uint32_t result = HL_value + SP_value;
			
        	// Store the result back in the HL register pair
        	HL = result; // Assuming HL is directly assignable
			
        	AF.setSubtract(false);
        	AF.setHalfCarry(((HL_value & 0x0FFF) + (SP_value & 0x0FFF)) > 0x0FFF);
        	AF.setCarry(result > 0xFFFF);
            
            return 8;
        }
		
        case 0x3E: {
            /**
             * LD A, u8
             * 2, 8
             */
            
            AF.A = mmu.fetch8(PC++);
            
            return 8;
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
            
            AF.setCarry(AF.A == 0);
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
            
            AF.setCarry(AF.A == 0);
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
            
            AF.setCarry(AF.A == 0);
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
            
            AF.setCarry(AF.A == 0);
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
            
            AF.setCarry(AF.A == 0);
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
            
            AF.setCarry(AF.A == 0);
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
            //AF.A ^= AF.A;
            
            AF.setCarry(AF.A == 0);
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
			
        	uint8_t result = AF.A - AF.A;
        	
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
            
        case 0xC3: {
            /**
             * JP a16
             * 3, 16
             */
            
            uint16_t address = mmu.fetch16(PC);
            
            // Jump to 'address' unconditionally
            PC = address;
            
            return 16;
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

        case 0xCD: {
            /**
             * CALL u16
             * 3, 24
             */
            
            pushToStack(PC++);
            
            uint16_t u16 = mmu.fetch16(PC++);
            PC = u16;
            
            return 24;
        }
		
		case 0xD0: {
			/**
			 * RET NC
			 * 1, 8 (20 with branch)
			 * - - - -
			 */
			
        	if(!AF.getCarry()) {
				uint16_t address = (mmu.fetch8(SP + 1) << 8) | mmu.fetch8(SP);
				
        		PC = address;
        		
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
        
        case 0xFF: {
            // RST 38H
            // Push the current PC onto the stack
            // 1, 16
            
            rst(0x38);
            
            return 16;
        }
        
        case 0xE0: {
            /*
             * LD (FF00 + U8), A
             * 2, 12
             */
            
            mmu.write8(0xFF00 + mmu.fetch8(PC++), AF.A);
            
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
        case 0x4f: // ld c,reg
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
        case 0x87: // add a,reg
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
            /*
            std::bitset<8> x(opcode);
            std::cout << x << '\n';
            */
             
            printf("Unknown instruction; %x\n", opcode);
            std::cerr << "";
            
            return 4;
        }
    }
}

void CPU::rst(uint16_t pc) {
    uint8_t high = (PC >> 8) & 0xFF;
    uint8_t low = PC & 0xFF;
    
    SP--;
    mmu.write8(SP, high);
    
    SP--;
    mmu.write8(SP, low);
    
    // Jump to the address 0x0038
    PC = pc;
}

void CPU::adc(uint8_t& regA, uint8_t& value, bool carry) {
    // Get the carry value (1 if carry is true, otherwise 0)
    uint8_t carryValue = carry ? 1 : 0;
    
    // Perform the addition with carry
    uint16_t result = regA + value + carryValue;
    
    AF.setZero((result & 0xFF) == 0);
    AF.setSubtract(false);
    AF.setHalfCarry(((regA & 0xF) + (value & 0xF) + carryValue) > 0xF);
    AF.setCarry(result > 0xFF);
    
    // Store the lower 8 bits of the result back in regA
    regA = result & 0xFF;
}

void CPU::pushToStack(uint16_t value) {
    mmu.write8(--SP, (value >> 8) & 0xFF);
    mmu.write8(--SP, value & 0xFF);
}

void CPU::reset() {
    // Initialize registers and memory
    /*A = F = B = C = D = E = H = L = 0;
    SP = 0xFFFE;
    PC = 0x0100;
    
    // Clear memory
    std::fill(std::begin(memory), std::end(memory), 0);*/
    
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
     /*// Helper to reset CPU state
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

    // Test for 0x39: ADD HL, SP
    resetCPU();
    HL = 0x1000;
    SP = 0x1000;
    decodeInstruction(0x39); // Opcode for ADD HL, SP
    assert(HL.get() == 0x2000);
    assert(SP == 0x1000); // SP should not be affected by this instruction

    // Test for 0x2A: LD A, (HL+)
    resetCPU();
    HL = 0x2000;
    mmu.write8(0x2000, 0xAB); // Write a test value to memory
    decodeInstruction(0x2A); // Opcode for LD A, (HL+)
    uint16_t f = HL.get();
    assert(AF.A == 0xAB);
    assert(HL.get() == 0x2001); // HL should increment

    // Test for 0xF3: DI
    resetCPU();
    decodeInstruction(0xF3); // Opcode for DI
    assert(interruptHandler.IME == false);
    
    // Test for 0x3C: INC A
    resetCPU();
    AF.A = 0x01;
    decodeInstruction(0x3C); // Opcode for INC A
    assert(AF.A == 0x02);

    // Test for 0xC9: RET
    resetCPU();
    SP = 0xC000;
    mmu.write8(SP, 0x34); // Set return address low byte
    mmu.write8(SP + 1, 0x12); // Set return address high byte
    decodeInstruction(0xC9); // Opcode for RET
    assert(PC == 0x1234);
    assert(SP == 0xC002); // Stack pointer should be restored

    // Test for 0xEA: LD (u16), A
    resetCPU();
    AF.A = 0x56;
    PC = 0x1000;
    mmu.write8(PC, 0x00); // Low byte of address
    mmu.write8(PC + 1, 0xC0); // High byte of address
    decodeInstruction(0xEA); // Opcode for LD (u16), A
    assert(mmu.fetch8(0xC000) == 0x56);

    // Test for 0x21: LD HL, u16
    resetCPU();
    PC = 0x1000;
    mmu.write8(PC, 0x34); // Low byte of address
    mmu.write8(PC + 1, 0x12); // High byte of address
    decodeInstruction(0x21); // Opcode for LD HL, u16
    assert(HL.get() == 0x1234);

    // Test for 0x20: JR NZ, i8
    resetCPU();
    PC = 0x1000;
    AF.setZero(false); // Zero flag not set
    mmu.write8(PC, 0xFE); // Offset for jump (relative)
    decodeInstruction(0x20); // Opcode for JR NZ, i8
    assert(PC == 0x1000 + 0xFE + 2); // PC should jump back to 0x1000
    
    // Test for 0xE5: PUSH HL
    resetCPU();
    HL = 0x1234;
    SP = 0xFFFE;
    decodeInstruction(0xE5); // Opcode for PUSH HL
    assert(mmu.fetch8(0xFFFE) == 0x12);
    assert(mmu.fetch8(0xFFFF) == 0x34);
    assert(SP == 0xFFFC);

    // Test for 0xF5: PUSH AF
    resetCPU();
    AF.A = 0x56;
    AF.F = 0x78;
    SP = 0xFFFE;
    decodeInstruction(0xF5); // Opcode for PUSH AF
    assert(mmu.fetch8(0xFFFE) == 0x56);
    assert(mmu.fetch8(0xFFFF) == 0x78);
    assert(SP == 0xFFFC);

    // Test for 0xC1: POP BC
    resetCPU();
    SP = 0xFFFE;
    mmu.write8(0xFFFE, 0x12); // Low byte of BC
    mmu.write8(0xFFFF, 0x34); // High byte of BC
    decodeInstruction(0xC1); // Opcode for POP BC
    assert(BC.C == 0x12);
    assert(BC.B == 0x34);
    assert(SP == 0x1000); // SP should be restored

    // Test for 0xF1: POP AF
    resetCPU();
    SP = 0xFFFE;
    mmu.write8(0xFFFE, 0x78); // Low byte of AF (F)
    mmu.write8(0xFFFF, 0x56); // High byte of AF (A)
    decodeInstruction(0xF1); // Opcode for POP AF
    assert(AF.F == 0x78);
    assert(AF.A == 0x56);
    assert(SP == 0x1000); // SP should be restored

    // Test for 0xFA: LD A, (u16)
    resetCPU();
    AF.A = 0x00;
    PC = 0x1000;
    mmu.write8(PC, 0x00); // Low byte of address
    mmu.write8(PC + 1, 0xC0); // High byte of address
    mmu.write8(0xC000, 0xAB); // Value at address
    decodeInstruction(0xFA); // Opcode for LD A, (u16)
    assert(AF.A == 0xAB);
    assert(PC == 0x1003); // PC should move past the instruction

    // Test for 0x23: INC HL
    resetCPU();
    HL = 0x1234;
    decodeInstruction(0x23); // Opcode for INC HL
    assert(HL.get() == 0x1235);

    // Test for 0x18: JR r8
    resetCPU();
    PC = 0x1000;
    mmu.write8(PC, 0xFE); // Offset for jump (relative)
    decodeInstruction(0x18); // Opcode for JR r8
    assert(PC == 0x1000 + 0xFE + 2); // PC should jump back to 0x1000*/
}
