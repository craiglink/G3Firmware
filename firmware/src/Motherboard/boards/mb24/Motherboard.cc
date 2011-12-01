/*
 * Copyright 2010 by Adam Mayer	 <adam@makerbot.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>
#include "Motherboard.hh"
#include "Configuration.hh"
#include "Steppers.hh"
#include "Command.hh"
#include "Interface.hh"
#include "Tool.hh"
#include "Commands.hh"
#include "Eeprom.hh"
#include "EepromMap.hh"


/// Instantiate static motherboard instance
Motherboard Motherboard::motherboard;


/// Create motherboard object
Motherboard::Motherboard() 
#if HAS_INTERFACE_BOARD > 0
    :
        interfaceBoard(
#if HAS_INTERFACE_BUTTONS > 0
            &mainMenu,
#else
            &monitorMode,
#endif // HAS_INTERFACE_BUTTONS > 0
            &monitorMode)

#endif // HAS_INTERFACE_BOARD > 0
{
}

#if STEPPER_COUNT > 0
StepperTmpltEndstops<0, X_DIR_PIN,X_STEP_PIN,X_ENABLE_PIN,X_MAX_PIN,X_MIN_PIN> Motherboard::stepperX;
#endif
#if STEPPER_COUNT > 1
StepperTmpltEndstops<1, Y_DIR_PIN,Y_STEP_PIN,Y_ENABLE_PIN,Y_MAX_PIN,Y_MIN_PIN> Motherboard::stepperY;
#endif
#if STEPPER_COUNT > 2
StepperTmpltEndstops<2, Z_DIR_PIN,Z_STEP_PIN,Z_ENABLE_PIN,Z_MAX_PIN,Z_MIN_PIN> Motherboard::stepperZ;
#endif
#if STEPPER_COUNT > 3
StepperTmplt<3, A_DIR_PIN,A_STEP_PIN,A_ENABLE_PIN> Motherboard::stepperA;
#endif
#if STEPPER_COUNT > 4
StepperTmplt<4, B_DIR_PIN,B_STEP_PIN,B_ENABLE_PIN> Motherboard::stepperB;
#endif
const StepperInterface* Motherboard::stepper[STEPPER_COUNT] =
{
	/// Set up the stepper pins on board creation
#if STEPPER_COUNT > 0
	&stepperX
#endif
#if STEPPER_COUNT > 1
	,&stepperY
#endif
#if STEPPER_COUNT > 2
	,&stepperZ
#endif
#if STEPPER_COUNT > 3
	,&stepperA
#endif
#if STEPPER_COUNT > 4
	,&stepperB // swap B for extruder before of lower port usage
#endif
};

/// Reset the motherboard to its initial state.
/// This only resets the board, and does not send a reset
/// to any attached toolheads.
void Motherboard::reset() {
	// Configure the debug pin.
	DEBUG_PIN::setDirection(true);

    DEBUG_MOTHERBOARD_SLICE_PIN::setDirection(true);
    DEBUG_MOTHERBOARD_SLICE_PIN::setValue(false);

	indicateError(0); // turn off blinker

	// Init steppers
	uint8_t axis_invert = eeprom::getEeprom8(eeprom::AXIS_INVERSION, 0);
	// Z holding indicates that when the Z axis is not in
	// motion, the machine should continue to power the stepper
	// coil to ensure that the Z stage does not shift.
	// Bit 7 of the AXIS_INVERSION eeprom setting
	// indicates whether or not to use z holding; 
	// the bit is active low. (0 means use z holding,
	// 1 means turn it off.)
	bool hold_z = (axis_invert & (1<<7)) == 0;
	steppers::setHoldZ(hold_z);

#if STEPPER_COUNT > 0
	stepperX.init();
#endif
#if STEPPER_COUNT > 1
	stepperY.init();
#endif
#if STEPPER_COUNT > 2
	stepperZ.init();
#endif
#if STEPPER_COUNT > 3
	stepperA.init();
#endif
#if STEPPER_COUNT > 4
	stepperB.init(); // swap B for extruder before of lower port usage
#endif

    // Initialize the host and slave UARTs
        UART::getHostUART().enable(true);
        UART::getHostUART().in.reset();
        UART::getSlaveUART().enable(true);
        UART::getSlaveUART().in.reset();
	// Reset and configure timer 1, the microsecond and stepper
	// interrupt timer.
	TCCR1A = 0x00;
	TCCR1B = 0x09;
	TCCR1C = 0x00;
	OCR1A = INTERVAL_IN_MICROSECONDS * 16;
	TIMSK1 = 0x02; // turn on OCR1A match interrupt
	// Reset and configure timer 2, the debug LED flasher timer.
	TCCR2A = 0x00;
	TCCR2B = 0x07; // prescaler at 1/1024
	TIMSK2 = 0x01; // OVF flag on
}

void Motherboard::initInterfaceBoard() {
#if HAS_INTERFACE_BOARD > 0
	// Check if the interface board is attached
    hasInterfaceBoard = interface::isConnected();
	if (hasInterfaceBoard) {
		// Make sure our interface board is initialized
        interfaceBoard.init();

        // Then add the splash screen to it.
        interfaceBoard.pushScreen(&splashScreen);

        // Finally, set up the *** interface
        interface::init(&interfaceBoard);

        interface_update_timeout.start(interfaceBoard.getUpdateRate());
	}
#endif // HAS_INTERFACE_BOARD > 0
}


/// Run the motherboard interrupt
void Motherboard::doInterrupt() {

	micros += INTERVAL_IN_MICROSECONDS;

#if HAS_INTERFACE_BOARD > 0
	if (hasInterfaceBoard) {
        interfaceBoard.doInterrupt();
	}
#endif // HAS_INTERFACE_BOARD > 0

    // Do not move steppers if the board is in a paused state
	if (command::isPaused()) return;
    	steppers::doInterrupt();

}

void Motherboard::runMotherboardSlice() {
    DEBUG_MOTHERBOARD_SLICE_PIN::setValue(true);

#if HAS_INTERFACE_BOARD > 0
	if (hasInterfaceBoard) {
		if (interface_update_timeout.hasElapsed()) {
            interfaceBoard.doUpdate();
            interface_update_timeout.start(interfaceBoard.getUpdateRate());
		}
	}
#endif

    DEBUG_MOTHERBOARD_SLICE_PIN::setValue(false);
}


/// Timer one comparator match interrupt
ISR(TIMER1_COMPA_vect) {
	Motherboard::getBoard().doInterrupt();
}

/// Number of times to blink the debug LED on each cycle
volatile uint8_t blink_count = 0;

/// The current state of the debug LED
enum {
	BLINK_NONE,
	BLINK_ON,
	BLINK_OFF,
	BLINK_PAUSE
} blink_state = BLINK_NONE;

/// Write an error code to the debug pin.
void Motherboard::indicateError(int error_code) {
	if (error_code == 0) {
		blink_state = BLINK_NONE;
		DEBUG_PIN::setValue(false);
	}
	else if (blink_count != error_code) {
		blink_state = BLINK_OFF;
	}
	blink_count = error_code;
}

/// Get the current error code.
uint8_t Motherboard::getCurrentError() {
	return blink_count;
}

/// Timer2 overflow cycles that the LED remains on while blinking
#define OVFS_ON 18
/// Timer2 overflow cycles that the LED remains off while blinking
#define OVFS_OFF 18
/// Timer2 overflow cycles between flash cycles
#define OVFS_PAUSE 80

/// Number of overflows remaining on the current blink cycle
int blink_ovfs_remaining = 0;
/// Number of blinks performed in the current cycle
int blinked_so_far = 0;

/// Timer 2 overflow interrupt
ISR(TIMER2_OVF_vect) {
	if (blink_ovfs_remaining > 0) {
		blink_ovfs_remaining--;
	} else {
		if (blink_state == BLINK_ON) {
			blinked_so_far++;
			blink_state = BLINK_OFF;
			blink_ovfs_remaining = OVFS_OFF;
			DEBUG_PIN::setValue(false);
		} else if (blink_state == BLINK_OFF) {
			if (blinked_so_far >= blink_count) {
				blink_state = BLINK_PAUSE;
				blink_ovfs_remaining = OVFS_PAUSE;
			} else {
				blink_state = BLINK_ON;
				blink_ovfs_remaining = OVFS_ON;
				DEBUG_PIN::setValue(true);
			}
		} else if (blink_state == BLINK_PAUSE) {
			blinked_so_far = 0;
			blink_state = BLINK_ON;
			blink_ovfs_remaining = OVFS_ON;
			DEBUG_PIN::setValue(true);
		}
	}
}
