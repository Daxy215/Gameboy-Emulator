#include <filesystem>
#include <fstream>
#include <SDL.h>
#include <sstream>
#include <thread>

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
 *
 * Main guide:
 * https://gbdev.io/pandocs/Specifications.html
 *
 * Used for debugging;
 * https://robertheaton.com/gameboy-doctor/
 * https://github.com/retrio/gb-test-roms/tree/master
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
    std::ofstream logFile("cpu_state_log.txt");
    
    if (!logFile.is_open()) {
        throw std::runtime_error("Unable to open log file.");
    }
    
    std::ifstream blarggFile("Roms/cpu_state.txt");
    if (!blarggFile.is_open()) {
        throw std::runtime_error("Unable to open cpu_state.txt.");
    }
    
    std::vector<std::string> blarggStates;
    std::string line;
    while (std::getline(blarggFile, line) && blarggStates.size() < 200) {
        blarggStates.push_back(line);
    }
    
    uint64_t x = 1;
    
    bool running = true;
    
    while (running) {
        if(x == 78813) {
            printf("");
        }
        
        // https://gbdev.io/pandocs/Interrupts.html#ime-interrupt-master-enable-flag-write-only
        if(cpu.ei >= 0) cpu.ei--;
        
        if(cpu.ei == 0) {
            cpu.interruptHandler.IME = true;
        }
        
        uint16_t cycles = cpu.interruptHandler.handleInterrupt(cpu);
        
        // If cycles are not 0 then an interrupt happend
        if(!cpu.halted && cycles == 0) {
            uint16_t opcode = cpu.fetchOpCode();
            cycles = cpu.decodeInstruction(opcode);
        } else if(cpu.halted) {
            if(cycles > 0) {
                printf("??");
            }
            
            // bc im too lazy
            /*if(x == 77915 || x == 78808 || x == 82434 || x == 83327 || x == 84103
                || x == 86936 || x == 88741 || x == 89631 || x == 91433 || x == 91525
                || x == 91617 || x == 93082 || x == 93174)*/
            /*if(sizeof(blarggFile) < x + 1 && blarggStates[x + 1].find("PC: 00:0048") != std::string::npos) {
                //std::cerr << "Found at; " << blarggStates[x + 1] << "\n";
                
                cpu.interruptHandler.IF |= 0x02;
            }*/
            
            cycles = 4;
        }
        
        timer.tick(cycles);
        ppu.tick(cycles);
        
        // Apply interrupts if any occured
        cpu.interruptHandler.IF |= timer.interrupt;
        timer.interrupt = 0;
        
        cpu.interruptHandler.IF |= ppu.lcdc.interrupt;
        ppu.lcdc.interrupt = 0;
        
        cpu.interruptHandler.IF |= ppu.interrupt;
        ppu.interrupt = 0;
        
        std::string formattedState = formatCPUState(cpu);
        logFile << formattedState << '\n';
        //std::cerr << formattedState << " - " << x << "\n";
        
        if (x < blarggStates.size()) {
            const std::string &expectedState = blarggStates[x];
            if (formattedState != expectedState) {
                /*std::cerr << "Mismatch at iteration " << x << ":\n";
                std::cerr << "Expected: " << expectedState << "\n";
                for(int i = 0; i < 1; i++) {
                    std::cerr << "Expected " << std::to_string(x + i) << ": " << blarggStates[x + i] << "\n";
                }
                
                std::cerr << "Actual  : " << formattedState << "\n";*/
                
                //break;
            }
        } else if(x > blarggStates.size()) {
            //std::cerr << "No more expected states to compare.\n";
            //break;
        }
        
        x++;
    }
}

std::optional<uint8_t> stdoutprinter(uint8_t value) {
    std::cerr << static_cast<char>(value);
    return std::nullopt;
}

int main(int argc, char* argv[]) {
    using std::ifstream;
    using std::ios;
    
    //std::string filename = "Roms/Tennis (World).gb";
    //std::string filename = "Roms/Tetris 2.gb";
    //std::string filename = "Roms/TETRIS.gb";
    //std::string filename = "Roms/Super Mario Land (JUE) (V1.1) [!].gb";
    std::string filename = "Roms/dmg-acid2.gb";
    
    //std::string filename = "Roms/window_y_trigger.gb";
    //std::string filename = "Roms/window_y_trigger_wx_offscreen.gb";
    
    //std::string filename = "Roms/testRom1.gb";
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
    
    //std::string filename = "Roms/halt_bug.gb"; // TODO;
    //std::string filename = "Roms/instr_timing/instr_timing.gb"; // Passed
    //std::string filename = "Roms/mem_timing/mem_timing.gb"; // TODO;
    //std::string filename = "Roms/mooneye/emulator-only/mbc1/bits_bank1.gb"; // TODO;
    
    ifstream stream(filename.c_str(), ios::binary | ios::ate);
    
    if (!stream.good()) {
        std::cerr << "Cannot read from file: " << filename << '\n';
    }
    
    auto fileSize = stream.tellg();
    stream.seekg(0, ios::beg);
    
    std::vector<uint8_t> memory(/*fileSize*/ 2 * 1024 * 1024);
    
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
    
    // Create MMU
    MMU mmu(interruptHandler, joypad, mbc, wram, hram, vram, lcdc, serial, timer, oam, *(new PPU(vram, oam, lcdc, mmu)), memory);
    
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
    
    emulationThread.join();
    
    // Cleanup code
    SDL_DestroyWindow(ppu->window);
    SDL_Quit();
    
    return 0;
}
