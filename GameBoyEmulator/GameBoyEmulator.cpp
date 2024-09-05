#include <bitset>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <SDL.h>
#include <SDL_events.h>
#include <sstream>
#include <thread>
#include <vector>

#include "Cartridge.h"
#include "CPU.h"

#include "InterrupHandler.h"

#include "HRAM.h"
#include "VRAM.h"
#include "WRAM.h"
#include "ExternalRAM.h"

#include "LCDC.h"
#include "Serial.h"

#include "MMU.h"
#include "OAM.h"
#include "PPU.h"

/*
 * GOOD GUIDES;
 *
 * A list of opcodes and their behaviors
 * https://rgbds.gbdev.io/docs/v0.8.0/gbz80.7
 * https://gb-archive.github.io/salvage/decoding_gbz80_opcodes/Decoding%20Gamboy%20Z80%20Opcodes.html
 * http://www.codeslinger.co.uk/pages/projects/gameboy/banking.html
 * https://robertovaccari.com/gameboy/memory_map.png
 *
 * Main guide:
 * https://gbdev.io/pandocs/Specifications.html
 *
 * Used for debugging;
 * https://robertheaton.com/gameboy-doctor/
 * https://github.com/retrio/gb-test-roms/tree/master
 */

// Function to format CPU state as a string
std::string formatCPUState(const CPU &cpu) {
    std::ostringstream oss;
    
    // Format the registers and SP, PC in hex with leading zeros
    oss << std::hex << std::setfill('0');
    oss << "A:" << std::setw(2) << static_cast<int>(cpu.AF.A) << ' ';
    oss << "F:" << std::setw(2) << static_cast<int>(cpu.AF.F) << ' ';
    oss << "B:" << std::setw(2) << static_cast<int>(cpu.BC.B) << ' ';
    oss << "C:" << std::setw(2) << static_cast<int>(cpu.BC.C) << ' ';
    oss << "D:" << std::setw(2) << static_cast<int>(cpu.DE.D) << ' ';
    oss << "E:" << std::setw(2) << static_cast<int>(cpu.DE.E) << ' ';
    oss << "H:" << std::setw(2) << static_cast<int>(cpu.HL.H) << ' ';
    oss << "L:" << std::setw(2) << static_cast<int>(cpu.HL.L) << ' ';
    oss << "SP:" << std::setw(4) << static_cast<int>(cpu.SP) << ' ';
    oss << "PC:" << std::setw(4) << static_cast<int>(cpu.PC) << ' ';
    
    // Fetch the memory around the PC
    uint16_t pc = cpu.PC;
    uint8_t mem[4];
    
    for (uint16_t i = 0; i < 4; i++) {
        mem[i] = cpu.mmu.fetch8(pc + i);
    }
    
    oss << "PCMEM:" << std::hex;
    for (int i = 0; i < 4; i++) {
        oss << std::setw(2) << static_cast<int>(mem[i]);
        if (i < 3) oss << ',';
    }
    
    return oss.str();
}

void runEmulation(CPU &cpu, PPU &ppu) {  // NOLINT(clang-diagnostic-missing-noreturn)
    std::ofstream logFile("cpu_state_log.txt");
    
    if (!logFile.is_open()) {
        throw std::runtime_error("Unable to open log file.");
    }
    
    // Log the current CPU state
    logFile << formatCPUState(cpu) << '\n';
    
    uint64_t x = 0;
    
    while (true) {
        uint16_t opcode = cpu.fetchOpCode();
        int cycles = cpu.decodeInstruction(opcode);
        
        // Log the current CPU state
        logFile << formatCPUState(cpu) << '\n';
        
        /*if(x == 16441) {
            printf("s");
        }*/
        
        x++;
        
        //printf("X; " + x);
        //std::cerr << "";
        
        //ppu.tick(cycles);
    }
}

int main(int argc, char* argv[]) {
    using std::ifstream;
    using std::ios;
    
    //std::string filename = "Roms/Tennis (World).gb";
    //std::string filename = "Roms/dmg-acid2.gb";
    //std::string filename = "Roms/cpu_instrs/cpu_instrs.gb";
    //std::string filename = "Roms/cpu_instrs/individual/01-special.gb";
    //std::string filename = "Roms/cpu_instrs/individual/07-jr,jp,call,ret,rst.gb";
    std::string filename = "Roms/cpu_instrs/individual/09-op r,r.gb";
    //std::string filename = "Roms/cpu_instrs/individual/10-bit ops.gb";
    
    ifstream stream(filename.c_str(), ios::binary | ios::ate);
    
    if (!stream.good()) {
        std::cerr << "Cannot read from file: " << filename << '\n';
    }
    
    auto fileSize = stream.tellg();
    stream.seekg(0, ios::beg);
    
    std::vector<uint8_t> memory(fileSize);
    
    if (!stream.read(reinterpret_cast<char*>(memory.data()), fileSize)) {
        std::cerr << "Error reading file!" << '\n';
        
        return 1;
    }
    
    InterruptHandler interruptHandler;
    
    // I/O
    LCDC lcdc;
    Serial serial;
    
    OAM oam;
    
    // Init memories
    WRAM wram;
    HRAM hram;
    VRAM vram(lcdc);
    ExternalRAM externalRam;
    
    PPU* ppu;
    
    // Create MMU
    MMU mmu(interruptHandler, wram, hram, vram, externalRam, lcdc, serial, oam, *(new PPU(vram, oam, lcdc, mmu)), memory);
    ppu = &mmu.ppu;
    
    Cartridge cartridge;
    
    CPU cpu(interruptHandler, mmu);
    
    // NO bios
    /*mmu.write8(0xFF40, 0x91);
    mmu.write8(0xFF47, 0xFC);
    mmu.write8(0xFF48, 0xFF);
    mmu.write8(0xFF49, 0xFF);*/
    
    // TODO; Test
    //cpu.testCases();
    
    ppu->createWindow();
    
    // Gather cartridge information
    cartridge.decode(memory);
    
    runEmulation(cpu, *ppu);
    
    /*std::thread emulationThread(runEmulation, std::ref(cpu), std::ref(*ppu));
    
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
        while (SDL_PollEvent(&e) != 0) {
            // User requests quit
            if (e.type == SDL_QUIT) {
                running = false;  // Exit the loop
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
    
    emulationThread.join();*/
    
    // Cleanup code
    SDL_DestroyWindow(ppu->window);
    SDL_Quit();
    
    return 0;
}
