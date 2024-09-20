#include "Joypad.h"

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
	
	return 0xFF;
	
	uint8_t result = P1 | 0xF;
	
	if (!(P1 & 0x20)) { // Bit 5 - Buttons mode selected
		result |= (buttonState | 0xF0); // Apply button presses to lower nibble
	} else if (!(P1 & 0x10)) { // Bit 4 - D-pad mode selected
		result |= (dpadState | 0xF0); // Apply d-pad presses to lower nibble
	}
	
	return result;
}

void Joypad::write8(uint16_t address, uint8_t data) {
	P1 = data;
}

void Joypad::pressButton(Buttons button) {
	buttonState &= ~(1 << button);
}

void Joypad::releaseButton(Buttons button) {
	buttonState |= (1 << button);
}

void Joypad::pressDpad(Dpad dpad) {
	dpadState &= ~(1 << dpad);
}

void Joypad::releaseDpad(Dpad dpad) {
	dpadState |= (1 << dpad);
}
