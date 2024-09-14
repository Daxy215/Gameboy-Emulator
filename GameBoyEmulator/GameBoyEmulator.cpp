#include <filesystem>
#include <fstream>
#include <SDL.h>
#include <sstream>
#include <thread>

#include "Memory/Cartridge.h"
#include "CPU/CPU.h"

#include "CPU/InterrupHandler.h"

#include "Memory/HRAM.h"
#include "Pipeline//VRAM.h"
#include "Memory/WRAM.h"
#include "Memory/ExternalRAM.h"

#include "Pipeline/LCDC.h"
#include "CPU/Serial.h"
#include "CPU/Timer.h"
#include "Memory/MBC/MBC.h"

#include "Memory/MMU.h"
#include "Memory/MBC/MBC.h"
#include "Pipeline/OAM.h"
#include "Pipeline/PPU.h"

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

// Function to format CPU state as a string
std::string formatCPUState(const CPU &cpu) {
    std::ostringstream oss;
    
    // Format the registers and SP, PC in hex with leading zeros
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
    while (std::getline(blarggFile, line) && blarggStates.size() < 950000) {
        blarggStates.push_back(line);
    }
    
    uint64_t x = 0;
    
    bool running = true;
    
    while (running) {
        if(x == 156431) {
            printf("");
        }
        
        std::string formattedState = formatCPUState(cpu);
        logFile << formattedState << '\n';
        //std::cerr << formattedState << " - " << x << "\n";
        
        if (x < blarggStates.size()) {
            const std::string &expectedState = blarggStates[x];
            if (formattedState != expectedState) {
                /*std::cerr << "Mismatch at iteration " << x << ":\n";
                std::cerr << "Expected: " << expectedState << "\n";
                std::cerr << "Actual  : " << formattedState << "\n";*/
                
                //break;
            }
        } else if(x > blarggStates.size()) {
            //std::cerr << "No more expected states to compare.\n";
            //break;
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
            
            cycles = 4;
        }
        
        timer.tick(cycles);
        ppu.tick(cycles);
        
        // Apply interrupts if any occured
        cpu.interruptHandler.IF |= timer.interrupt;
        timer.interrupt = 0;
        
        cpu.interruptHandler.IF |= ppu.interrupt;
        ppu.interrupt = 0;
        
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
    
    std::string filename = "Roms/Tennis (World).gb";
    //std::string filename = "Roms/Tetris 2.gb";
    //std::string filename = "Roms/dmg-acid2.gb";
    
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
    Serial serial;
    Timer timer;
    
    OAM oam;
    
    // Init memories
    WRAM wram;
    HRAM hram;
    VRAM vram(lcdc);
    ExternalRAM externalRam;
    
    PPU* ppu;
    
    // Create MMU
    MMU mmu(interruptHandler, mbc, wram, hram, vram, externalRam, lcdc, serial, timer, oam, *(new PPU(vram, oam, lcdc, mmu)), memory);
    
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
    
    emulationThread.join();
    
    // Cleanup code
    SDL_DestroyWindow(ppu->window);
    SDL_Quit();
    
    return 0;
}
