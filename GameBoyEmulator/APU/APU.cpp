#include "APU.h"

#include "../Utility/Bitwise.h"

void APU::tick(uint32_t cycles) {
	
}

uint8_t APU::fetch8(uint16_t address) {
	return 0;
}

void APU::write8(uint16_t address, uint8_t data) {
	/**
	 * According to: https://gbdev.io/pandocs/Audio_Registers.html#ff26--nr52-audio-master-control
	 *
	 * Turning the APU off drains less power (around 16%),
	 * but clears all APU registers and makes them read-only until turned back on, except NR521.
	 */
	if(!enabled) {
		return;
	}
	
	if(address == 0xFF26) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff26--nr52-audio-master-control
		
		// Bit 7 - Audio on/off
		enabled = check_bit(data, 7);
		
		// Bit 3 - CH4 on?
		ch4.enabled = check_bit(data, 4);
		
		// Bit 2 - CH3 on?
		ch3.enabled = check_bit(data, 4);
		
		// Bit 1 - CH2 on?
		ch2.enabled = check_bit(data, 4);
		
		// Bit 0 - CH1 on?
		ch1.enabled = check_bit(data, 4);
	} else if(address == 0xFF25) {
		// 	https://gbdev.io/pandocs/Audio_Registers.html#ff25--nr51-sound-panning
		
		// Bit 7 - CH4 Left
		ch4.left = check_bit(data, 7);

		// Bit 6 - CH3 Left
		ch3.left = check_bit(data, 6);

		// Bit 5 - CH2 Left
		ch2.left = check_bit(data, 5);

		// Bit 4 - CH1 Left
		ch1.left = check_bit(data, 4);

		// Bit 3 - CH4 Right
		ch4.right = check_bit(data, 3);
		
		// Bit 2 - CH3 Right
		ch3.right = check_bit(data, 2);
		
		// Bit 1 - CH2 Right
		ch2.right = check_bit(data, 1);
		
		// Bit 0 - CH4 Right
		ch1.right = check_bit(data, 0);
	} else if(address == 0xFF24) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff24--nr50-master-volume--vin-panning

		// Bit 7 - VIN Left
		vinLeft = check_bit(data, 7);

		// Bit 7, 5 and 4 - Left volume
		leftVolume = (data & 0b01110000) >> 4;
		
		// Bit 3
		vinRight = check_bit(data, 3);
		
		// Bit 2, 1 and 0 - Right volume
		rightVolume = data & 0b00000111;
	} else if(address == 0xFF10) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff10--nr10-channel-1-sweep
		
		/**
		 * Bit 6, 5 and 4 - Pace
		 *
		 * TODO; Note that the value written to this field is not re-read by the hardware,
		 * until a sweep iteration completes, or the channel is (re)triggered.
		 *
		 * However, if 0 is written to this field,
		 * then iterations are instantly disabled (but see below),
		 * and it will be reloaded as soon as it’s set to something else.
		 */
		ch1.pace = (data & 0b01110000) >> 4;
		
		/**
		 * Bit 3 - Direction
		 *
		 * 0 - Addition (period increases)
		 * 1 - Subreaction (period decreases)
		 */
		ch1.direction = check_bit(data, 3);
		
		// Bit 2, 1 and 0 - Individual step
		ch1.individualStep = (data & 0b00000111);
	} else if(address == 0xFF11) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff11--nr11-channel-1-length-timer--duty-cycle
		
		// Bit 7 and 6 - Wave duty
		ch1.waveDuty = (data & 0b11000000) >> 6;
		
		/**
		 * Bit 5 to 0 - Initial length timer
		 *	
		 * This is a write only!
		 * https://gbdev.io/pandocs/Audio.html#length-timer
		*/
		ch1.lengthTimer = data & 0b00111111;
	} else if(address == 0xFF12) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff12--nr12-channel-1-volume--envelope
		
		/** 
		 * Bit 7 through 4 - Initial volume
		 * 
		 * R/W - Cannot be updated anywhere else!	
		 */
		ch1.initialVolume = (data & 0b11110000) >> 4;
		
		/**
		 * Bit 3 - Env dir
		 *
		 * Envelope direction
		 * 0 = Decrease volume over time
		 * 1 = Increase volume over time
		 */
		ch1.envDir = check_bit(data, 3);
		
		// Bit 2, 1 and 0 - Sweep pace
		ch1.sweepPace = data & 0b00000111;
		
		/**
		 * TODO; Setting bits 3-7 to 0,
		 * turns off DAC and the channel as well!
		 */
	} else if(address == 0xFF13) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff13--nr13-channel-1-period-low-write-only
		
		ch1.periodLow = data;
	} else if(address == 0xFF14) {
		// https://gbdev.io/pandocs/Audio_Registers.html#ff14--nr14-channel-1-period-high--control
		
		/**
		 * Bit 7 - Trigger Write-only
		 *
		 * Writting any value to NR14(this address),
		 * with this bet being set, it triggers the channel.
		 */
		ch1.trigger = check_bit(data, 7);
		
		// Bit 6 - Length enable - R/W
		ch1.lengthEnable = check_bit(data, 6);

		/**
		 * Bit 2, 1 and 0 - Period
		 *
		 * This is the upper 3 bits of the period value;
		 * the lower 8 bits are from NR13 (ch1.periodLow)
		 */
		ch1.periodHigh = data & 0b00000111;
	} else if(address == 0xFF15) {
		// https://gbdev.io/pandocs/Audio_Registers.html#sound-channel-2--pulse
		
		/*
		 * Channel 2 doesn't have sweep.
		 * So I suppose ignore this?
		 *
		 * TODO; Check this
		 */
	} else if(address == 0xFF16) {
		// NR21, exact same behavior as NR11 but for CH2.
		
		// https://gbdev.io/pandocs/Audio_Registers.html#ff11--nr11-channel-1-length-timer--duty-cycle
		
		// Bit 7 and 6 - Wave duty
		ch2.waveDuty = (data & 0b11000000) >> 6;
		
		/**
		 * Bit 5 to 0 - Initial length timer
		 *	
		 * This is a write only!
		 * https://gbdev.io/pandocs/Audio.html#length-timer
		*/
		ch2.lengthTimer = data & 0b00111111;
	} else if(address == 0xFF17) {
		// NR22, exact same behavior as NR12 but for CH2.
		
		// https://gbdev.io/pandocs/Audio_Registers.html#ff12--nr12-channel-1-volume--envelope
		
		/** 
		 * Bit 7 through 4 - Initial volume
		 * 
		 * R/W - Cannot be updated anywhere else!	
		 */
		ch2.initialVolume = (data & 0b11110000) >> 4;
		
		/**
		 * Bit 3 - Env dir
		 *
		 * Envelope direction
		 * 0 = Decrease volume over time
		 * 1 = Increase volume over time
		 */
		ch2.envDir = check_bit(data, 3);
		
		// Bit 2, 1 and 0 - Sweep pace
		ch2.sweepPace = data & 0b00000111;
		
		/**
		 * TODO; Setting bits 3-7 to 0,
		 * turns off DAC and the channel as well!
		 */
	} else if(address == 0xFF18) {
		// NR23, exact same behavior as NR13 but for CH2.
		
		// https://gbdev.io/pandocs/Audio_Registers.html#ff13--nr13-channel-1-period-low-write-only
		
		ch2.periodLow = data;
	} else if(address == 0xFF19) {
		// NR24, exact same behavior as NR14 but for CH2.
		
		// https://gbdev.io/pandocs/Audio_Registers.html#ff14--nr14-channel-1-period-high--control
		
		/**
		 * Bit 7 - Trigger Write-only
		 *
		 * Writting any value to NR14(this address),
		 * with this bet being set, it triggers the channel.
		 */
		ch2.trigger = check_bit(data, 7);
		
		// Bit 6 - Length enable - R/W
		ch2.lengthEnable = check_bit(data, 6);
		
		/**
		 * Bit 2, 1 and 0 - Period
		 *
		 * This is the upper 3 bits of the period value;
		 * the lower 8 bits are from NR13 (ch1.periodLow)
		 */
		ch2.periodHigh = data & 0b00000111;
	}
}
