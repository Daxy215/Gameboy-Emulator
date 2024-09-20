#pragma once
#include <cstdint>

// https://gbdev.io/pandocs/Joypad_Input.html

enum Buttons {
	START = 0x08,
	SELECT = 0x04,
	B = 0x02,
	A = 0x01
};

enum Dpad {
	DOWN = 0x08,
	UP = 0x04,
	LEFT = 0x02,
	RIGHT = 0x01
};

class Joypad {
public:
	uint8_t fetch8(uint16_t address);
	void write8(uint16_t address, uint8_t data);
	
	void pressButton(Buttons button);
	void releaseButton(Buttons button);
	void pressDpad(Dpad dpad);
	void releaseDpad(Dpad dpad);
	
private:
	// All buttons pressed by default
	uint8_t P1 = 0xFF;
	
	uint8_t buttonState = 0x0F;
	uint8_t dpadState = 0x0F;
};
