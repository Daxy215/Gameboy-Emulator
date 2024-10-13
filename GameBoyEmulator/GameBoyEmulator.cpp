#include <filesystem>
#include <fstream>
#include <SDL.h>
#include <sstream>
#include <thread>
#include <chrono>
#include <thread>

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

#include "APU/APU.h"
#include "backends/imgui_impl_sdlrenderer2.h"
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

// TODO; Clean this class up..

std::string toLowerCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
    return result;
}

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
    oss << " LY: " << static_cast<int>(cpu.mmu.lcdc.LY);
    
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

// TODO; Remove
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
    //std::string filename = "Roms/Tetris 2.gb";
    //std::string filename = "Roms/Super Mario Land (JUE) (V1.1) [!].gb";
    //std::string filename = "Roms/Super Mario Land 2 - 6 Golden Coins (UE) (V1.2) [!].gb";
    //std::string filename = "Roms/Super Mario Land 4 (J) [!].gb";
    //std::string filename = "Roms/Mario & Yoshi (E) [!].gb";
    
    // Not fully tested but seems ok - though has some rendering issues
    // Idk what I did but now it's completely fucked
    //std::string filename = "Roms/SpongeBob SquarePants - Legend of the Lost Spatula (U) [C][!].gbc"; // Uses MBC5
    
    /**
     * Fixing the issue with,
     * WRAM and HRAM, fixed,
     * some of the rendering issues.
     * 
     * Future me; I was being too stupid,
     * WRAM and VRAM's sizes were too small.
     */
    
    /**
     * This game had up arrow stuck and rendering issues,
     * it was bc I had an issue with MBC
     *
     * So, issue is back idk how it got fixed in the first place.
     * From my understanding, input is registered via interrupt,
     * when pressing up arrow, it doesn't cause an interrupt.
     * 
     * Though, every other button it does?
     * 
     * Also, it's quite weird that this is ONLY happening,
     * in this game. Every other game, it works correctly.
     */
    //std::string filename = "Roms/Pokemon Green (U) [p1][!].gb"; // TODO; Up arrow stuck
    //std::string filename = "Roms/Legend of Zelda, The - Link's Awakening DX (U) (V1.2) [C][!].gbc"; // Uses MBC5
    //std::string filename = "Roms/Mario Golf (U) [C][!].gbc"; // Uses MBC5
    //std::string filename = "Roms/Mario Tennis (U) [C][!].gbc"; // Uses MBC5
    
    /**
     * Seems to work but has some rendering issues
     */
    //std::string filename = "Roms/Super Mario Bros. Deluxe (U) (V1.1) [C][!].gbc";
    
    /*
     * TODO; I believe they require speed switch IRQ
     * Future me - It isn't the issue after all. Idk y it'd be xD
     */
    
    /*
     * Ok I think the issue is the halt bug..
     * Which I can find little resources of ;)
     * 
     * Or maybe HDMA? I don't know why I thought it'd be HDMA.. it's for CGB
     * 
     * Turns out it was the MBC1 :D
     * Still a bit glitchy.
     * 
     * Seems, like if you die in zelda,
     * the lighting uses the wrong texture map..
     */
    std::string filename = "Roms/Legend of Zelda, The - Link's Awakening (U) (V1.2) [!].gb";
    //std::string filename = "Roms/Amazing Spider-Man 2, The (UE) [!].gb";
    //std::string filename = "Roms/Yu-Gi-Oh! Duel Monsters (J) [S].gb";
    
    /**
     * It runs but then freezes;
     * 
     * Issue was with 0xFF55 returning wrong value.
     *
     * Runs correctly now
     */
    //std::string filename = "Roms/Disney's Tarzan (U) [C][!].gbc"; // Uses MBC5
    
    // Games that don't work :(
    // TODO; These do run, just don't go past the main screen. Probably bc MBC3 is wrongly implemented(I just copied MBC1)
    //std::string filename = "Roms/Pokemon TRE Team Rocket Edition (Final).gb"; // Uses MBC3
    //std::string filename = "Roms/Pokemon Red (UE) [S][!].gb"; // Uses MBC3
    //std::string filename = "Roms/Pokemon - Blue Version (UE) [S][!].gb"; // Uses MBC3..
    //std::string filename = "Roms/Pokemon - Crystal Version (UE) (V1.1) [C][!].gbc"; // Uses MBC3..
    //std::string filename = "Roms/Pokemon - Yellow Version (UE) [C][!].gbc"; // Uses MBC3..
    
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
    //std::string filename = "Roms/tests/blargg/instr_timing/instr_timing.gb"; // Passed
    //std::string filename = "Roms/tests/blargg/interrupt_time/interrupt_time.gb"; // TODO;
    
    // MEMORY TIMING
    //std::string filename = "Roms/mem_timing/mem_timing.gb"; // TODO;
    //std::string filename = "Roms/mem_timing/individual/01-read_timing.gb"; // TODO;
    //std::string filename = "Roms/mem_timing/individual/02-write_timing.gb"; // TODO;
    //std::string filename = "Roms/mem_timing/individual/03-modify_timing.gb"; // TODO;
    
    //std::string filename = "Roms/tests/bully/bully.gb"; // TODO; DMA bus conflict always reads $FF??
    
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
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/div_timing.gb"; // Passed
    
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/bits/mem_oam.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/bits/reg_f.gb"; // Passed I think?
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/bits/unused_hwio-GS.gb"; // TODO;

    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/instr/daa.gb"; // Passed
    
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/interrupts/ie_push.gb"; // TODO;
    
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/oam_dma/basic.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/oam_dma/reg_read.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/acceptance/oam_dma/sources-GS.gb"; // TODO; Failed BF00
    
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
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/multicart_rom_8MB.gb"; // TODO; Wrong bank number
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/ram_64kb.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/ram_256kb.gb"; // Passed
    
    // TODO; These gets stuck if boot rom is disabled.
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/rom_1Mb.gb"; // TODO; Wrong bank number
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/rom_2Mb.gb"; // TODO; Wrong bank number
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/rom_4Mb.gb"; // Passed
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/rom_8Mb.gb"; // TODO; Wrong bank number
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/rom_16Mb.gb"; // TODO;  Wrong bank number
    //std::string filename = "Roms/tests/mooneye-test-suite/emulator-only/mbc1/rom_512kb.gb"; // TODO; Wrong bank number
    
    //std::string filename = "Roms/tests/mooneye-test-suite/manual-only/sprite_priority.gb"; // Passed
    
    // An MBC3 Test for the real time clock
    //std::string filename = "Roms/tests/rtc3test/rtc3test.gb"; // Uses MBC3
    
    //std::string filename = "Roms/tests/same-suite/apu/div_trigger_volume_10.gb";
    
    //std::string filename = "Roms/tests/scribbltests/lycscx/lycscx.gb"; // Passed
    //std::string filename = "Roms/tests/scribbltests/lycscy/lycscy.gb"; // Passed
    //std::string filename = "Roms/tests/scribbltests/palettely/palettely.gb"; // Passed
    //std::string filename = "Roms/tests/scribbltests/scxly/scxly.gb"; // Passed
    //std::string filename = "Roms/tests/scribbltests/statcount/statcount.gb"; // TODO??
    //std::string filename = "Roms/tests/scribbltests/winpos/winpos.gb"; // Passed
    
    //std::string filename = "Roms/tests/strikethrough/strikethrough.gb"; // Passed
    
    //std::string filename = "Roms/tests/turtle-tests/window_y_trigger/window_y_trigger.gb"; // Passed
    //std::string filename = "Roms/tests/turtle-tests/window_y_trigger_wx_offscreen/window_y_trigger_wx_offscreen.gb"; // Passed
    
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
    /**
     * I have no clue what's happening,
     * but writting to hram sometimes,
     * fucks up wram??? And vice versa..
     * 
     * I am so confused rn.
     * 
     * Future me; I was being too stupid,
     * WRAM and VRAM's sizes were too small.
     */
    WRAM wram;
    HRAM hram;
    
    VRAM vram(lcdc);
    
    PPU* ppu;
    APU apu;
    
    // Create MMU
    MMU mmu(interruptHandler, serial, joypad, mbc, wram,
        hram, vram, lcdc, timer, oam,
        *(new PPU(vram, oam, lcdc, mmu)),
        apu, bootDMG, memory);
    
    // Listen.. I'm too lazy to deal with this crap
    ppu = &mmu.ppu;
    
    CPU cpu(interruptHandler, mmu);
    
    serial.set_callback(stdoutprinter);
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL could not initialize SDL_Error: " << SDL_GetError() << '\n';
        return -1;
    }
    
    ppu->createWindow();
    apu.init();
    
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();
    
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    
    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(ppu->window, ppu->renderer);
    ImGui_ImplSDLRenderer2_Init(ppu->renderer);
    
    // Load save
    mmu.mbc.load("Saves/" + cartridge.title + "/save.bin");
    
    bool running = true;
    
    const double CLOCK_SPEED_NORMAL = 4194304; // 4.194304 MHz
    const double CLOCK_SPEED_DOUBLE = 8388608; // 8.388608 MHz
    const int FPS = 60;
    
    // Time tracking variables
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    
    uint16_t frames = 0;
    
    /**
     * Idk if I'm doing something wrong but,
     * many tests are failling because timer,
     * is not meant to increament for a bit?
     * 
     * 425 - Seems to allow it to pass,
     * the DIV_TIMING test rom
     */
    //int32_t timerDelay = 425;
    
    /*
     * But to pass bully test rom,
     * 425 doesn't work.. ;-;
     */
    int32_t timerDelay = 256;
    
    while (running) {
        double cyclesPerFrame = cpu.mmu.doubleSpeed ? CLOCK_SPEED_DOUBLE / FPS : CLOCK_SPEED_NORMAL / FPS;
        
        uint64_t totalCyclesThisFrame = 0;
        
        SDL_Event e;
        
        //SDL_RenderClear(ppu->renderer);
        //SDL_RenderCopy(ppu->renderer, ppu->texture, nullptr, nullptr);
        
        // Handle events on queue
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            
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
        
        while (totalCyclesThisFrame < cyclesPerFrame) {
            uint16_t cycles = cpu.cycle();
            
            if(!cpu.mmu.bootRomActive) {
                cpu.mmu.tick(cycles);
                
                if(timerDelay > 0)
                    timerDelay -= cycles;
                
                if(timerDelay <= 0)
                    timer.tick(cycles * (cpu.mmu.doubleSpeed ? 2 : 1));
            }
            
            cpu.mmu.apu.tick(cycles);
            ppu->tick(cycles / (cpu.mmu.doubleSpeed ? 2 : 1) + cpu.mmu.cycles);
            
            cpu.interruptHandler.IF |= timer.interrupt;
            timer.interrupt = 0;
            
            cpu.interruptHandler.IF |= cpu.mmu.joypad.interrupt;
            cpu.mmu.joypad.interrupt = 0;
            
            cpu.interruptHandler.IF |= ppu->interrupt;
            ppu->interrupt = 0;
            
            cpu.interruptHandler.IF |= cpu.mmu.serial.interrupt;
            cpu.mmu.serial.interrupt = 0;
            
            totalCyclesThisFrame += cycles;
            cpu.mmu.cycles = 0;
        }
        
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        
        ImGui::Begin("Game Boy");
            ImGui::Image((void*)(intptr_t)ppu->texture, ImVec2(256*2, 256*2));
        ImGui::End();
        
        ImGui::Begin("Pulse Waveform");
            ImGui::Text(("Duty cycles: " + std::to_string(apu.ch1.waveDuty) + " = " + std::to_string(apu.ch1.sequencePointer) + " = " + std::to_string(apu.ch1.currentVolume) + "/" + std::to_string(apu.ch1.initialVolume)).c_str());
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            const ImVec2 windowPos = ImGui::GetCursorScreenPos();
            const float width = 400;
            const float height = 200;
            
            // Draw background
            drawList->AddRectFilled(windowPos, ImVec2(windowPos.x + width, windowPos.y + height), IM_COL32(30, 30, 30, 255));
            
            // Get samples for rendering
            const auto& samples = cpu.mmu.apu.newSamples;
            
            // Draw waveform
            for (size_t i = 0; i < samples.size() - 1; ++i) {
                float x1 = windowPos.x + (i * width / (samples.size() - 1));
                float y1 = windowPos.y + height / 2 - (samples[i] / 255.0f * height / 2);
                float x2 = windowPos.x + ((i + 1) * width / (samples.size() - 1));
                float y2 = windowPos.y + height / 2 - (samples[i + 1] / 255.0f * height / 2);
                
                drawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), IM_COL32(255, 255, 255, 255));
            }
        
        ImGui::End();
        
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
            SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
        }
        
        ImGui::Render();
        SDL_RenderClear(ppu->renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), ppu->renderer);
        SDL_RenderPresent(ppu->renderer);
        
        //glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        //glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        //glClear(GL_COLOR_BUFFER_BIT);
        //ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        //SDL_GL_SwapWindow(ppu->window);
        
        //SDL_RenderClear(ppu->renderer);
        //SDL_RenderCopy(ppu->renderer, ppu->texture, NULL, NULL);
        //SDL_RenderPresent(ppu->renderer);
        
        frames++;
        
        // Calculate how much time should have passed for this frame
        auto now = std::chrono::high_resolution_clock::now();
        auto frameTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameTime).count();
        
        // Frame time should be 1000 / FPS milliseconds (16.67 ms at 60 FPS)
        double targetFrameTime = 1000.0 / FPS;
        
        // Sleep to limit frame rate if we are running too fast
        if (frameTime < targetFrameTime) {
            frames = 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(targetFrameTime - frameTime)));
        }
        
        lastFrameTime = now;
    }
    
    mbc.save("Saves/" + cartridge.title + "/save.bin");
    
    // Cleanup code
    SDL_DestroyWindow(ppu->window);
    SDL_Quit();
    
    return 0;
}
