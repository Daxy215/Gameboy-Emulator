#include "Joypad.h"

#include "../Utility/Bitwise.h"

/**
 * PLEASE NOTE;
 *
 * This isn't made by me at all!
 * I got it from: https://github.com/jgilchrist/gbemu/blob/becc2e4475c2ba765b43701a8211f06d61b21219/src/input.cc#L9
 * 
 * Just wanted a quick working Joypad..
 */

uint8_t Joypad::fetch8(uint16_t address) {
	/**
	 * https://gbdev.io/pandocs/Joypad_Input.html#ff00--p1joyp-joypad
	 * 
	 * Select buttons: If this bit is 0,
	 * then buttons (SsBA) can be read from the lower nibble.
     * 
	 * Select d-pad: If this bit is 0,
	 * then directional keys can be read from the lower nibble.
	 * 
	 * The lower nibble is Read-only. Note that,
	 * rather unconventionally for the Game Boy,
	 * a button being pressed is seen as the corresponding bit being 0, not 1.
	 * 
	 * If neither buttons nor d-pad is selected ($30 was written),
	 * then the low nibble reads $F (all buttons released).
	 */
	
	/*
	 * If neither buttons nor d-pad is selected ($30 was written),
	 * then the low nibble reads $F (all buttons released).
	 */
	uint8_t buttons = 0b1111;
	
	if (direction_switch) {
		buttons = set_bit_to(buttons, 0, !right);
		buttons = set_bit_to(buttons, 1, !left);
		buttons = set_bit_to(buttons, 2, !up);
		buttons = set_bit_to(buttons, 3, !down);
	}
	
	if (button_switch == true) {
		buttons = set_bit_to(buttons, 0, !a);
		buttons = set_bit_to(buttons, 1, !b);
		buttons = set_bit_to(buttons, 2, !select);
		buttons = set_bit_to(buttons, 3, !start);
	}
	
	buttons = set_bit_to(buttons, 4, !direction_switch);
	buttons = set_bit_to(buttons, 5, !button_switch);
	
	return buttons;
}

void Joypad::write8(uint16_t address, uint8_t data) {
	direction_switch = !check_bit(data, 4);
	button_switch = !check_bit(data, 5);
}

void Joypad::checkForInterrupts() {
	uint8_t current_buttons_state = 0;
	uint8_t current_dpad_state = 0;
	
	// Get current button states
	if (button_switch) {
		current_buttons_state |= (!a ? 1 : 0);
		current_buttons_state |= (!b ? 2 : 0);
		current_buttons_state |= (!select ? 4 : 0);
		current_buttons_state |= (!start ? 8 : 0);
	}
	
	// Get current D-pad states
	if (direction_switch) {
		current_dpad_state |= (!up ? 1 : 0);
		current_dpad_state |= (!down ? 2 : 0);
		current_dpad_state |= (!left ? 4 : 0);
		current_dpad_state |= (!right ? 8 : 0);
	}
	
	bool inter = false;
	
	// Check for transitions in buttons
	for (int i = 0; i < 4; ++i) {
		if ((current_buttons_state & (1 << i)) == 0 && (prev_buttons_state & (1 << i)) != 0) {
			inter = true;
		}
	}
	
	// Check for transitions in d-pad
	for (int i = 0; i < 4; ++i) {
		if ((current_dpad_state & (1 << i)) == 0 && (prev_dpad_state & (1 << i)) != 0) {
			inter = true;
		}
	}
	
	if(inter)
		interrupt |= 0x10;
	
	// Update previous states for next check
	prev_buttons_state = current_buttons_state;
	prev_dpad_state = current_dpad_state;
}

void Joypad::pressButton(Buttons button) {
	if (button == Buttons::A) { a = true; }
	if (button == Buttons::B) { b = true; }
	if (button == Buttons::SELECT) { select = true; }
	if (button == Buttons::START) { start = true; }
	
	checkForInterrupts();
}

void Joypad::releaseButton(Buttons button) {
	if (button == Buttons::A) { a = false; }
	if (button == Buttons::B) { b = false; }
	if (button == Buttons::SELECT) { select = false; }
	if (button == Buttons::START) { start = false; }
	
	checkForInterrupts();
}

void Joypad::pressDpad(Dpad dpad) {
	if (dpad == UP) { up = true; }
	if (dpad == DOWN) { down = true; }
	if (dpad == LEFT) { left = true; }
	if (dpad == RIGHT) { right = true; }
	
	checkForInterrupts();
}

void Joypad::releaseDpad(Dpad dpad) {
	if (dpad == UP) { up = false; }
	if (dpad == DOWN) { down = false; }
	if (dpad == LEFT) { left = false; }
	if (dpad == RIGHT) { right = false; }
	
	checkForInterrupts();
}
