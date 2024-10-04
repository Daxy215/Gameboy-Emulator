#include <filesystem>
#include <fstream>
#include <SDL.h>
#include <sstream>
#include <thread>
#include <chrono>
#include <thread>

#include "APU/APU.h"
#include "CPU/CPU.h"

#include "IO/InterrupHandler.h"
#include "IO/Joypad.h"
#include "IO/Serial.h"
#include "IO/Timer.h"

#include "Memory/Cartridge.h"
#include "Memory/MMU.h"
#include "Memory/HRAM.h"
#include "Memory/WRAM.h"
#include "Memory/MBC/MBC.h"

#include "Pipeline/PPU.h"
#include "Pipeline/LCDC.h"
#include "Pipeline//VRAM.h"
#include "Pipeline/OAM.h"

/*
 * GOOD GUIDES;
 *
 * A list of opcodes and their behaviors
 * https://rgbds.gbdev.io/docs/v0.8.0/gbz80.7
 * https://gb-archive.github.io/salvage/decoding_gbz80_opcodes/Decoding%20Gamboy%20Z80%20Opcodes.html
 * http://www.codeslinger.co.uk/pages/projects/gameboy/banking.html
 * https://robertovaccari.com/gameboy/memory_map.png
 * https://blog.tigris.fr/2019/09/15/writing-an-emulator-the-first-pixel/
 * -
 * Helped with understanding blargg's test roms:
 * https://github.com/retrio/gb-test-roms/blob/master/
 *
 * Helped with MBCS:
 * https://github.com/Gekkio/gb-ctr
 * https://gekkio.fi/files/gb-docs/gbctr.pdf
 *
 * Main guide:
 * https://gbdev.io/pandocs/Specifications.html
 *
 * Used for debugging;
 * https://robertheaton.com/gameboy-doctor/
 * https://github.com/retrio/gb-test-roms/tree/master
 *
 * Other stuff;
 * https://stackoverflow.com/questions/38730273/how-to-limit-fps-in-a-loop-with-c
 */

// TODO; https://gbdev.io/pandocs/Power_Up_Sequence.html#hardware-registers

// Function to format CPU state as a string
std::string formatCPUState(const CPU &cpu) {
    std::ostringstream oss;
    
    oss << std::hex << std::uppercase << std::setfill('0');
    oss << "A: " << std::setw(2) << static_cast<int>(cpu.AF.A) << ' ';
    oss << "F: " << std::setw(2) << static_cast<int>(cpu.AF.F) << ' ';
    oss << "B: " << std::setw(2) << static_cast<int>(cpu.BC.B) << ' ';
    oss << "C: " << std::setw(2) << static_cast<int>(cpu.BC.C) << ' ';
    oss << "D: " << std::setw(2) << static_cast<int>(cpu.DE.D) << ' ';
    oss << "E: " << std::setw(2) << static_cast<int>(cpu.DE.E) << ' ';
    oss << "H: " << std::setw(2) << static_cast<int>(cpu.HL.H) << ' ';
    oss << "L: " << std::setw(2) << static_cast<int>(cpu.HL.L) << ' ';
    oss << "SP: " << std::setw(4) << static_cast<int>(cpu.SP) << ' ';
    oss << "PC: 00:" << std::setw(4) << static_cast<int>(cpu.PC) << ' ';
    oss << "IME: " << (cpu.interruptHandler.IME ? "true" : "false") ;
    
    // Fetch the memory around the PC
    uint16_t pc = cpu.PC;
    uint8_t mem[4];
    
    for (uint16_t i = 0; i < 4; i++) {
        mem[i] = cpu.mmu.fetch8(pc + i);
    }
    
    oss << "(" << std::hex;
    for (int i = 0; i < 4; i++) {
        oss << std::setw(2) << static_cast<int>(mem[i]);
        if (i < 3) oss << ' ';
    }
    oss << ")" << std::hex;
    
    return oss.str();
}

void runEmulation(CPU& cpu, PPU& ppu, Timer& timer) {
    uint32_t stopCycles = 0;
    
    bool running = true;
    
    const double CLOCK_SPEED_NORMAL = 4194304; // 4.194304 MHz
    const double CLOCK_SPEED_DOUBLE = 8388608; // 8.388608 MHz
    const int FPS = 60; // Frames per second
    
    // Time tracking variables
    auto lastFrameTime = std::chrono::high_resolution_clock::now();

    uint32_t x = 0;

    while (running) {
        double cyclesPerFrame = cpu.mmu.doubleSpeed ? CLOCK_SPEED_DOUBLE / FPS : CLOCK_SPEED_NORMAL / FPS;
        
        // Track the total cycles executed this frame
        uint64_t totalCyclesThisFrame = 0;
        
        while (totalCyclesThisFrame < cyclesPerFrame) {
            if(cpu.ei >= 0) cpu.ei--;
            
            if(cpu.ei == 0) {
                cpu.interruptHandler.IME = true;
            }
            
            uint16_t cycles = cpu.interruptHandler.handleInterrupt(cpu);
            
            if (!cpu.halted && !cpu.stop && cycles == 0) {
                uint16_t opcode = cpu.fetchOpCode();
                cycles = cpu.decodeInstruction(opcode);
                
                if (cpu.PC >= 0x0100) {
                    cpu.mmu.bootRomActive = false;
                }
            } else if (cpu.halted) {
                if (cycles > 0) {
                    printf("??");
                }
                cycles = 4;
            }
            
            if (cpu.stop) {
                stopCycles += (cycles == 0) ? 4 : cycles;
                if (stopCycles >= 8200) {
                    stopCycles = 0;
                    cpu.stop = false;
                }
            } else {
                timer.tick(cycles * (cpu.mmu.doubleSpeed ? 2 : 1));
            }
            
            x++;
            
            cpu.mmu.tick();
            ppu.tick(cycles);
            
            cpu.interruptHandler.IF |= timer.interrupt;
            timer.interrupt = 0;
            
            cpu.interruptHandler.IF |= cpu.mmu.joypad.interrupt;
            cpu.mmu.joypad.interrupt = 0;
            
            cpu.interruptHandler.IF |= ppu.interrupt;
            ppu.interrupt = 0;
            
            cpu.interruptHandler.IF |= cpu.mmu.serial.interrupt;
            cpu.mmu.serial.interrupt = 0;
            
            totalCyclesThisFrame += cycles;
        }
        
        // Calculate how much time should have passed for this frame
        auto now = std::chrono::high_resolution_clock::now();
        auto frameTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameTime).count();
        
        // Frame time should be 1000 / FPS milliseconds (16.67 ms at 60 FPS)
        double targetFrameTime = 1000.0 / FPS;
        
        // Sleep to limit frame rate if we are running too fast
        if (frameTime < targetFrameTime) {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(targetFrameTime - frameTime)));
        }
        
        lastFrameTime = now;
    }
}

std::optional<uint8_t> stdoutprinter(uint8_t value) {
    std::cerr << static_cast<char>(value);
    return std::nullopt;
}

int main(int argc, char* argv[]) {
    using std::ifstream;
    using std::ios;
    
    // GAMES
    
    // Games that work
    //std::string filename = "Roms/Tennis (World).gb";
    //std::string filename = "Roms/TETRIS.gb";
    //std::string filename = "Roms/Super Mario Land (JUE) (V1.1) [!].gb";
    //std::string filename = "Roms/Super Mario Land 2 - 6 Golden Coins (UE) (V1.2) [!].gb";
    //std::string filename = "Roms/Mario & Yoshi (E) [!].gb";
    
    // Games that don't work :(
    //std::string filename = "Roms/Pokemon TRE Team Rocket Edition (Final).gb"; // Uses MBC3
    //std::string filename = "Roms/Pokemon Red (UE) [S][!].gb"; // Uses MBC3
    //std::string filename = "Roms/Pokemon - Blue Version (UE) [S][!].gb"; // Uses MBC3..
    
    //std::string filename = "Roms/Pokemon Green (U) [p1][!].gb"; // TODO; Up arrow stuck??
    //std::string filename = "Roms/Legend of Zelda, The - Link's Awakening DX (U) (V1.2) [C][!].gbc"; // Uses MBC5
    //std::string filename = "Roms/Mario Golf (U) [C][!].gbc"; // Uses MBC5
    //std::string filename = "Roms/Mario Tennis (U) [C][!].gbc"; // Uses MBC5
    //std::string filename = "Roms/SpongeBob SquarePants - Legend of the Lost Spatula (U) [C][!].gbc"; // Uses MBC5
    
    /*
     * TODO; I believe they require speed switch IRQ
     * Future me - It isn't the issue after all. Idk y it'd be xD
     */
    
    /*
     * Ok I think the issue is the halt bug..
     * Which I can find little resources of ;)
     * 
     * Or maybe HDMA?
     * 
     * Turns out it was the MBC1 :D
     * Still a bit glitchy
     */
    std::string filename = "Roms/Legend of Zelda, The - Link's Awakening (U) (V1.2) [!].gb";
    //std::string filename = "Roms/Amazing Spider-Man 2, The (UE) [!].gb";
    //std::string filename = "Roms/Yu-Gi-Oh! Duel Monsters (J) [S].gb";
    
    // TESTS
    
    //std::string filename = "Roms/dmg-acid2.gb"; // Passed
    //std::string filename = "Roms/tests/cgb-acid2/cgb-acid2.gbc"; // TODO; Nose not showing
    
    //std::string filename = "Roms/testRom1.gb"; // Idk? Ig passed
    
    //std::string filename = "Roms/tests/age-test-roms/vram/vram-read-cgbBCE.gb"; // TODO: ?
    
    // CPU INSTRUCTIONS
    //std::string filename = "Roms/cpu_instrs/cpu_instrs.gb"; // Passed
    //std::string filename = "Roms/cpu_instrs/individual/01-special.gb"; // Passed
    //std::string filename = "Roms/cpu_instrs/individual/02-interrupts.gb"; // Passed
    //std::string filename = "Roms/cpu_instrs/individual/03-op sp,hl.gb"; // Passed
    //std::string filename = "Roms/cpu_instrs/individual/04-op r,imm.gb"; // Passed
    //std::string filename = "Roms/cpu_instrs/individual/05-op rp.gb"; // Passed
    //std::string filename = "Roms/cpu_instrs/individual/06-ld r,r.gb"; // Passed
    //std::string filename = "Roms/cpu_instrs/individual/07-jr,jp,call,ret,rst.gb"; // Passed
    //std::string filename = "Roms/cpu_instrs/individual/08-misc instrs.gb"; // Passed
    //std::string filename = "Roms/cpu_instrs/individual/09-op r,r.gb"; // Passed
    //std::string filename = "Roms/cpu_instrs/individual/10-bit ops.gb"; // Passed
    //std::string filename = "Roms/cpu_instrs/individual/11-op a,(hl).gb"; // Passed
    
    // Sounds
    //std::string filename = "Roms/tests/blargg/dmg_sound/dmg_sound.gb"; // TODO;
    //std::string filename = "Roms/tests/blargg/dmg_sound/rom_singles/01-registers.gb"; // TODO;
    //std::string filename = "Roms/tests/blargg/dmg_sound/rom_singles/02-len ctr.gb"; // TODO;
    //std::string filename = "Roms/tests/blargg/dmg_sound/rom_singles/03-trigger.gb"; // TODO;
    //std::string filename = "Roms/tests/blargg/dmg_sound/rom_singles/04-sweep.gb"; // TODO;
    //std::string filename = "Roms/tests/blargg/dmg_sound/rom_singles/05-sweep details.gb"; // TODO;
    //std::string filename = "Roms/tests/blargg/dmg_sound/rom_singles/06-overflow on trigger.gb"; // TODO;
    //std::string filename = "Roms/tests/blargg/dmg_sound/rom_singles/07-len sweep period sync.gb"; // TODO;
    //std::string filename = "Roms/tests/blargg/dmg_sound/rom_singles/08-len ctr during power.gb"; // TODO;
    //std::string filename = "Roms/tests/blargg/dmg_sound/rom_singles/09-wave read while on.gb"; // TODO;
    //std::string filename = "Roms/tests/blargg/dmg_sound/rom_singles/10-wave trigger while on.gb"; // TODO;
    //std::string filename = "Roms/tests/blargg/dmg_sound/rom_singles/11-regs after power.gb"; // TODO;
    //std::string filename = "Roms/tests/blargg/dmg_sound/rom_singles/12-wave write while on.gb"; // TODO;
    
    //std::string filename = "Roms/halt_bug.gb"; // TODO;
    //std::string filename = "Roms/instr_timing/instr_timing.gb"; // Passed
    //std::string filename = "Roms/tests/blargg/interrupt_time/interrupt_time.gb"; // TODO;
    
    // MEMORY TIMING
    //std::string filename = "Roms/mem_timing/mem_timing.gb"; // TODO;
    //std::string filename = "Roms/mem_timing/individual/01-read_timing.gb"; // TODO;
    //std::string filename = "Roms/mem_timing/individual/02-write_timing.gb"; // TODO;
    //std::string filename = "Roms/mem_timing/individual/03-modify_timing.gb"; // TODO;
    
    //std::string filename = "Roms/tests/bully/bully.gb"; // TODO;
    
    //std::string filename = "Roms/tests/little-things-gb/tellinglys.gb"; // Passed
    
    // OAM BUG
    //std::string filename = "Roms/tests/blargg/oam_bug/oam_bug.gb"; // TODO;
    //std::string filename = "Roms/tests/blargg/oam_bug/rom_singles/1-lcd_sync.gb"; // Passed
    //std::string filename = "Roms/tests/blargg/oam_bug/rom_singles/2-causes.gb"; // TODO;
    //std::string filename = "Roms/tests/blargg/oam_bug/rom_singles/3-non_causes.gb"; // Passed
    //std::string filename = "Roms/tests/blargg/oam_bug/rom_singles/4-scanline_timing.gb"; // TODO;
    //std::string filename = "Roms/tests/blargg/oam_bug/rom_singles/5-timing_bug.gb"; // TODO;
    //std::string filename = "Roms/tests/blargg/oam_bug/rom_singles/6-timing_no_bug.gb"; // Passed
    //std::string filename = "Roms/tests/blargg/oam_bug/rom_singles/7-timing_effect.gb"; // TODO;
    //std::string filename = "Roms/tests/blargg/oam_bug/rom_singles/8-instr_effect.gb"; // TODO;
    
    //std::string filename = "Roms/tests/mbc3-fiddle/mbc3.gb"; // TODO;
    //std::string filename = "Roms/tests/mbc3-fiddle/mbc3-withram.gb"; // TODO;
     
    //std::string filename = "Roms/tests/mbc3-tester/mbc3-tester.gb"; // TODO;
    
    // Mooneye
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/bits/mem_oam.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/bits/reg_f.gb"; // Passed I think?
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/bits/unused_hwio-GS.gb"; // TODO;

    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/instr/daa.gb"; // Passed
    
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/interrupts/ie_push.gb"; // TODO;
    
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/oam_dma/basic.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/oam_dma/reg_read.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/oam_dma/sources-GS.gb"; // TODO; Uses MBC5
    
    // Timer
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/timer/div_write.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/timer/rapid_toggle.gb"; // TODO;
    
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/timer/tim00.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/timer/tim00_div_trigger.gb"; // TODO;
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/timer/tim01.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/timer/tim01_div_trigger.gb"; // TODO;
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/timer/tim10.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/timer/tim10_div_trigger.gb"; // TODO;
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/timer/tim11.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/timer/tim11_div_trigger.gb"; // TODO;
    
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/timer/tima_reload.gb"; // TODO;
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/timer/tima_write_reloading.gb"; // TODO;
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/timer/tma_write_reloading.gb"; // TODO;
    
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/bits_bank1.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/bits_bank2.gb"; // TOOD; It passed but doesn't show up?
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/bits_mode.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/bits_ramg.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/multicart_rom_8MB.gb"; // TODO;
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/ram_64kb.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/ram_256kb.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/rom_1Mb.gb"; // TODO; Stuck
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/rom_2Mb.gb"; // TODO; Stuck
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/rom_4Mb.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/rom_8Mb.gb"; // TODO; Wrong bank number
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/rom_16Mb.gb"; // TODO;  Wrong bank number
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/rom_512kb.gb"; // TODO; Stuck
    
    //std::string filename = "Roms/tests/mooneye-test-suite/manual-only/sprite_priority.gb"; // Passed
    
    // An MBC3 Test for the real time clock
    //std::string filename = "Roms/tests/rtc3test/rtc3test.gb"; // Uses MBC3
    
    //std::string filename = "Roms/tests/scribbltests/lycscx/lycscx.gb"; // Passed
    //std::string filename = "Roms/tests/scribbltests/lycscy/lycscy.gb"; // Passed
    //std::string filename = "Roms/tests/scribbltests/palettely/palettely.gb"; // Passed
    //std::string filename = "Roms/tests/scribbltests/scxly/scxly.gb"; // Passed
    //std::string filename = "Roms/tests/scribbltests/statcount/statcount.gb"; // TODO??
    //std::string filename = "Roms/tests/scribbltests/winpos/winpos.gb"; // Idk?
    
    //std::string filename = "Roms/tests/strikethrough/strikethrough.gb"; // Passed
    
    //std::string filename = "Roms/tests/turtle-tests/window_y_trigger/window_y_trigger.gb"; // Passed
    //std::string filename = "Roms/tests/turtle-tests/window_y_trigger_wx_offscreen/window_y_trigger_wx_offscreen.gb"; // Passed
    
    ifstream stream(filename.c_str(), ios::binary | ios::ate);
    
    if (!stream.good()) {
        std::cerr << "Cannot read from file: " << filename << '\n';
    }
    
    auto fileSize = stream.tellg();
    stream.seekg(0, ios::beg);
    
    std::vector<uint8_t> memory(fileSize/* 2 * 1024 * 1024*/);
    
    if (!stream.read(reinterpret_cast<char*>(memory.data()), fileSize)) {
        std::cerr << "Error reading file!" << '\n';
        
        return 1;
    }

    // TODO; Make a class for this
    /*std::string booRom = "Roms/bootroms/dmg0_boot.bin";
    
    ifstream bootStream(filename.c_str(), ios::binary | ios::ate);
    
    if (!bootStream.good()) {
        std::cerr << "Cannot read from file: " << booRom << '\n';
    }
    
    auto size = bootStream.tellg();
    bootStream.seekg(0, ios::beg);
    
    std::vector<uint8_t> bootRom(size);
    
    if (!bootStream.read(reinterpret_cast<char*>(bootRom.data()), size)) {
        std::cerr << "Error reading file!" << '\n';
        
        return 1;
    }*/
    
    // From GearBoy
    std::vector<uint8_t> bootDMG = {
        0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32, 0xCB, 0x7C, 0x20, 0xFB, 0x21, 0x26, 0xFF, 0x0E,
        0x11, 0x3E, 0x80, 0x32, 0xE2, 0x0C, 0x3E, 0xF3, 0xE2, 0x32, 0x3E, 0x77, 0x77, 0x3E, 0xFC, 0xE0,
        0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1A, 0xCD, 0x95, 0x00, 0xCD, 0x96, 0x00, 0x13, 0x7B,
        0xFE, 0x34, 0x20, 0xF3, 0x11, 0xD8, 0x00, 0x06, 0x08, 0x1A, 0x13, 0x22, 0x23, 0x05, 0x20, 0xF9,
        0x3E, 0x19, 0xEA, 0x10, 0x99, 0x21, 0x2F, 0x99, 0x0E, 0x0C, 0x3D, 0x28, 0x08, 0x32, 0x0D, 0x20,
        0xF9, 0x2E, 0x0F, 0x18, 0xF3, 0x67, 0x3E, 0x64, 0x57, 0xE0, 0x42, 0x3E, 0x91, 0xE0, 0x40, 0x04,
        0x1E, 0x02, 0x0E, 0x0C, 0xF0, 0x44, 0xFE, 0x90, 0x20, 0xFA, 0x0D, 0x20, 0xF7, 0x1D, 0x20, 0xF2,
        0x0E, 0x13, 0x24, 0x7C, 0x1E, 0x83, 0xFE, 0x62, 0x28, 0x06, 0x1E, 0xC1, 0xFE, 0x64, 0x20, 0x06,
        0x7B, 0xE2, 0x0C, 0x3E, 0x87, 0xE2, 0xF0, 0x42, 0x90, 0xE0, 0x42, 0x15, 0x20, 0xD2, 0x05, 0x20,
        0x4F, 0x16, 0x20, 0x18, 0xCB, 0x4F, 0x06, 0x04, 0xC5, 0xCB, 0x11, 0x17, 0xC1, 0xCB, 0x11, 0x17,
        0x05, 0x20, 0xF5, 0x22, 0x23, 0x22, 0x23, 0xC9, 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
        0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
        0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
        0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E, 0x3C, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x3C,
        0x21, 0x04, 0x01, 0x11, 0xA8, 0x00, 0x1A, 0x13, 0xBE, 0x00, 0x00, 0x23, 0x7D, 0xFE, 0x34, 0x20,
        0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xFB, 0x86, 0x00, 0x00, 0x3E, 0x01, 0xE0, 0x50
    };
    
    ifstream boot("Roms/bootroms/cgb_boot.bin", ios::binary | ios::ate);
     
    if (!boot.good()) {
        std::cerr << "Cannot read from file: Roms/bootroms/cgb_boot.bin \n";
    }
    
    auto f = boot.tellg();
    boot.seekg(0, ios::beg);
    
    std::vector<uint8_t> bootCGB(f);
    
    if (!boot.read(reinterpret_cast<char*>(bootCGB.data()), f)) {
        std::cerr << "Error reading file!" << '\n';
        
        return 1;
    }
    
    Cartridge cartridge;
    
    // Gather cartridge information
    cartridge.decode(memory);
    MBC mbc(cartridge, memory);
    
    InterruptHandler interruptHandler;
    
    // I/O
    LCDC lcdc;
    Joypad joypad;
    Serial serial;
    Timer timer;
    
    OAM oam;
    
    // Init memories
    WRAM wram;
    HRAM hram;
    VRAM vram(lcdc);
    
    PPU* ppu;
    APU apu;
    
    // Create MMU
    MMU mmu(interruptHandler, serial, joypad, mbc, wram, hram, vram, lcdc, timer, oam, *(new PPU(vram, oam, lcdc, mmu)), apu, bootDMG, memory);
    
    // Listen.. I'm too lazy to deal with this crap
    ppu = &mmu.ppu;
    
    CPU cpu(interruptHandler, mmu);
    
    serial.set_callback(stdoutprinter);
    
    // NO bios
    /*mmu.write8(0xFF40, 0x91);
    mmu.write8(0xFF47, 0xFC);
    mmu.write8(0xFF48, 0xFF);
    mmu.write8(0xFF49, 0xFF);*/
    
    // TODO; Test
    //cpu.testCases();
    
    ppu->createWindow();
    
    //runEmulation(cpu, *ppu);
    
    std::thread emulationThread(runEmulation, std::ref(cpu), std::ref(*ppu), std::ref(timer));

    // Boot rom
    for(int i = 0; i < 100; i++) {
        
    }
    
    bool running = true;
    const int targetFPS = 60;
    const int frameDelay = 1000 / targetFPS; // Frame delay in milliseconds
    
    uint32_t frameStart;
    uint32_t frameTime;
    
    while (running) {
        frameStart = SDL_GetTicks();  // Get the number of milliseconds since the SDL library was initialized
        
        // Event handler
        SDL_Event e;
        
        // Handle events on queue
        while (SDL_PollEvent(&e)) {
            // User requests quit
            if (e.type == SDL_QUIT) {
                running = false;  // Exit the loop
            }
            
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                case SDLK_RETURN: // Start
                        joypad.pressButton(START);
                        break;
                    case SDLK_BACKSPACE: // Select
                        joypad.pressButton(SELECT);
                        break;
                    case SDLK_a: // A button
                        joypad.pressButton(A);
                        break;
                    case SDLK_s: // B button
                        joypad.pressButton(B);
                        break;
                    case SDLK_UP: // D-pad up
                        joypad.pressDpad(UP);
                        break;
                    case SDLK_DOWN: // D-pad down
                        joypad.pressDpad(DOWN);
                        break;
                    case SDLK_LEFT: // D-pad left
                        joypad.pressDpad(LEFT);
                        break;
                    case SDLK_RIGHT: // D-pad right
                        joypad.pressDpad(RIGHT);
                        break;
                }
            }
            else if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.sym) {
                    case SDLK_RETURN: // Start
                        joypad.releaseButton(START);
                        break;
                    case SDLK_BACKSPACE: // Select
                        joypad.releaseButton(SELECT);
                        break;
                    case SDLK_a: // A button
                        joypad.releaseButton(A);
                        break;
                    case SDLK_s: // B button
                        joypad.releaseButton(B);
                        break;
                    case SDLK_UP: // D-pad up
                        joypad.releaseDpad(UP);
                        break;
                    case SDLK_DOWN: // D-pad down
                        joypad.releaseDpad(DOWN);
                        break;
                    case SDLK_LEFT: // D-pad left
                        joypad.releaseDpad(LEFT);
                        break;
                    case SDLK_RIGHT: // D-pad right
                        joypad.releaseDpad(RIGHT);
                        break;
                    }
            }
        }
        
        //SDL_RenderClear(ppu.renderer);
        
        // Swap the window buffer (update the window with the rendered image)
        //SDL_GL_SwapWindow(ppu.window);
        
        // Calculate how long the frame has taken
        frameTime = SDL_GetTicks() - frameStart;
        
        // If the frame finished early, delay the next iteration to maintain 60 FPS
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }
    
    emulationThread.detach();
    
    // Cleanup code
    //SDL_DestroyWindow(ppu->window);
    SDL_Quit();
    
    return 0;
}
