#include <filesystem>
#include <fstream>
#include <SDL.h>
#include <sstream>
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
    /*oss << "PC = " <<  std::setw(2) << static_cast<int>(cpu.PC) << ' ';
    oss << "Just executed=" <<  std::setw(1) << static_cast<int>(cpu.mmu.fetch8(cpu.PC)) << ' ';
    
    oss << "AF: " << std::setw(1) << static_cast<int>(cpu.AF.A) << ' ';
    oss  << std::setw(2) << static_cast<int>(cpu.AF.F) << ' ';
    
    oss << "BC: " << std::setw(1) << static_cast<int>(cpu.BC.B) << ' ';
    oss  << std::setw(2) << static_cast<int>(cpu.BC.C) << ' ';
    
    oss << "HL: " << std::setw(2) << static_cast<int>(cpu.HL.get()) << ' ';
    
    oss << "DE: " << std::setw(1) << static_cast<int>(cpu.DE.D) << ' ';
    oss  << std::setw(2) << static_cast<int>(cpu.DE.E) << ' ';
    
    oss << "SP: " << std::setw(2) << static_cast<int>(cpu.SP) << ' ';
    oss << " PC: " << std::setw(2) << static_cast<int>(cpu.PC);*/
    
    // Format the registers and SP, PC in hex with leading zeros
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
    
    // Fetch the memory around the PC
    uint16_t pc = cpu.PC;
    uint8_t mem[4];
    
    for (uint16_t i = 0; i < 4; i++) {
        mem[i] = cpu.mmu.fetch8(pc + i);
    }
    
    /*oss << "PCMEM:" << std::hex;
    for (int i = 0; i < 4; i++) {
        oss << std::setw(2) << static_cast<int>(mem[i]);
        if (i < 3) oss << ',';
    }*/
    
    oss << "(" << std::hex;
    for (int i = 0; i < 4; i++) {
        oss << std::setw(2) << static_cast<int>(mem[i]);
        if (i < 3) oss << ' ';
    }
    oss << ")" << std::hex;
    
    return oss.str();
}

void runEmulation(CPU& cpu, PPU& ppu, Timer& timer) {
    const std::chrono::nanoseconds targetFrameDuration(100);
    int frameCount = 0;
    auto lastFpsTime = std::chrono::high_resolution_clock::now();
    
    uint32_t stopCycles = 0;
    
    bool running = true;
    
    while (running) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // https://gbdev.io/pandocs/Interrupts.html#ime-interrupt-master-enable-flag-write-only
        if(cpu.ei >= 0) cpu.ei--;
        
        if(cpu.ei == 0) {
            cpu.interruptHandler.IME = true;
        }
        
        uint16_t cycles = cpu.interruptHandler.handleInterrupt(cpu);
        
        // If cycles are not 0, then an interrupt happened
        if(!cpu.halted && !cpu.stop && cycles == 0) {
            uint16_t opcode = cpu.fetchOpCode();
            cycles = cpu.decodeInstruction(opcode);
            
            /*std::string format = formatCPUState(cpu);
            std::cerr << format << " Opcode: " << std::hex << opcode << "\n";*/
            
            /*cycles += cpu.mmu.cycles;
            cpu.mmu.cycles = 0;*/
        } else if(cpu.halted) {
            if(cycles > 0) {
                printf("??");
            }
            
            cycles = 4;
        }
        
        if(cpu.stop) {
            stopCycles += cycles == 0 ? 4 : cycles;
            
            // CPU will pause for 8200 T cycles
            if(stopCycles >= 8200) {
                stopCycles = 0;
                cpu.stop = false;
            }
        } else {
            timer.tick(cycles * (cpu.mmu.doubleSpeed ? 2 : 1));
        }
        
        cpu.mmu.joypad.checkForInterrupts();
        ppu.tick(cycles);
        
        // Again I'm lazy
        //cpu.mmu.apu.tick(cycles);
        
        // Apply interrupts if any occured
        cpu.interruptHandler.IF |= timer.interrupt;
        timer.interrupt = 0;
        
        cpu.interruptHandler.IF |= cpu.mmu.joypad.interrupt;
        cpu.mmu.joypad.interrupt = 0;
        
        cpu.interruptHandler.IF |= ppu.lcdc.interrupt;
        ppu.lcdc.interrupt = 0;
        
        cpu.interruptHandler.IF |= ppu.interrupt;
        ppu.interrupt = 0;
        
        cpu.interruptHandler.IF |= cpu.mmu.serial.interrupt;
        cpu.mmu.serial.interrupt = 0;
        
        frameCount++;
        
        auto end = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        if (frameDuration < targetFrameDuration) {
            std::this_thread::sleep_for(targetFrameDuration - frameDuration); // Sleep for the remaining time
        }
        
        // FPS calculation
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastFpsTime).count() >= 1) {
            //std::cout << "FPS: " << frameCount << '\n';
            frameCount = 0;
            lastFpsTime = now;
        }
    }
}

std::optional<uint8_t> stdoutprinter(uint8_t value) {
    std::cerr << static_cast<char>(value);
    return std::nullopt;
}

int main(int argc, char* argv[]) {
    using std::ifstream;
    using std::ios;
    
    //std::string filename = "Roms/bootroms/dmg_boot.bin";
    
    // GAMES
    
    // Games that works
    //std::string filename = "Roms/Tennis (World).gb";
    //std::string filename = "Roms/TETRIS.gb";
    //std::string filename = "Roms/Super Mario Land (JUE) (V1.1) [!].gb";
    
    // Games that don't work :(
    //std::string filename = "Roms/Super Mario Land 2 - 6 Golden Coins (UE) (V1.2) [!].gb"; // Idek man
    //std::string filename = "Roms/Pokemon TRE Team Rocket Edition (Final).gb"; // Uses MBC3
    //std::string filename = "Roms/Pokemon Red (UE) [S][!].gb"; // Uses MBC3
    //std::string filename = "Roms/Pokemon - Blue Version (UE) [S][!].gb"; // Uses MBC3..
    //std::string filename = "Roms/Mario Golf (U) [C][!].gbc"; // Uses MBC5
    //std::string filename = "Roms/Mario Golf (U) [C][!].gbc"; // Uses MBC5
    
    /*
     * TODO; I believe they require speed switch IRQ
     * Future me - It isn't the issue after all. Idk y it'd be xD
     */
    
    /*
     * Ok I think the issue is the halt bug..
     * Which I can find little resources of ;)
     */
    //std::string filename = "Roms/Legend of Zelda, The - Link's Awakening (U) (V1.2) [!].gb";
    //std::string filename = "Roms/Amazing Spider-Man 2, The (UE) [!].gb";
    //std::string filename = "Roms/Yu-Gi-Oh! Duel Monsters (J) [S].gb"; // Just gets stuck?
    
    // TESTS
    
    //std::string filename = "Roms/dmg-acid2.gb"; // Passed
    //std::string filename = "Roms/tests/cgb-acid2/cgb-acid2.gbc"; // TODO;
    
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
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/instr/daa.gb"; // Passed
    
    // std::string filename = "Roms/tests/mooneye-test-suite/acceptance/oam_dma/basic.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/oam_dma/reg_read.gb"; // TODO;
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/oam_dma/sources-GS.gb"; // TODO; Uses MBC5
    
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/timer/div_write.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/timer/rapid_toggle.gb"; // TODO;
    
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/bits_bank1.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/bits_bank2.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/bits_mode.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/bits_ramg.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/multicart_rom_8MB.gb"; // TODO;
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/ram_64kb.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/ram_256kb.gb"; // TODO;
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/rom_1Mb.gb"; // TODO;
    
    //std::string filename = "Roms/tests/mooneye-test-suite/manual-only/sprite_priority.gb"; // TODO;
    
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
    MMU mmu(interruptHandler, joypad, mbc, wram, hram, vram, lcdc, serial, timer, oam, *(new PPU(vram, oam, lcdc, mmu)), apu, memory);
    
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
