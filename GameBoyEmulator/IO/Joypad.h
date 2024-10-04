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

	void checkForInterrupts();
	
	void pressButton(Buttons button);
	void releaseButton(Buttons button);
	void pressDpad(Dpad dpad);
	void releaseDpad(Dpad dpad);

public:
	uint8_t interrupt = 0;
	
private:
	bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool a = false;
    bool b = false;
    bool select = false;
    bool start = false;
	
    bool button_switch = false;
    bool direction_switch = false;
	
	uint8_t prev_buttons_state = 0x0F;
	uint8_t prev_dpad_state = 0x0F;
};
