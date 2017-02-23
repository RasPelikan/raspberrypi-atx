#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdint.h>

#include "util.h"
#include "uart.h"

#define NO_EVENT 0
#define BUTTON_PRESSED 1
#define BUTTON_RELEASED 2
#define RASPBERRYPI_SHUTDOWN 2

static volatile uint8_t portBHistory = 0x00;
static char event = NO_EVENT;

ISR(TIMER0_COMPA_vect) {

	process_timer();

}

/*
 * PCINT1-interrupt - needed for "wake up on pin-change"
 */
ISR(PCINT0_vect) {

	uint8_t currentPortB = PINB;
	uint8_t changedBits = currentPortB ^ portBHistory;
	portBHistory = currentPortB;

	if (event != NO_EVENT) {
		return;
	}

	if (is_bit_high(changedBits, PB1)) {

		if (is_bit_low(currentPortB, PB1)) {
			event = BUTTON_PRESSED;
		} else {
			event = BUTTON_RELEASED;
		}

	}

	if (is_bit_high(changedBits, PB4)) {

		if (is_bit_low(currentPortB, PB4)) {
			event = RASPBERRYPI_SHUTDOWN;
		}

	}

}

/*
 * bring MCU into hibernate
 */
static void go_asleep() {

	event = NO_EVENT;

	/*
	 * initialize "wake up on pin-change" on B1 (button) and B4 (Raspberry Pi off)
	 */
	set_bit(PCICR, PCIE0);                            // enable pin change interrupt
	sei();                                            // enable interrupt since this method might
	                                                  // be called within an interrupt and if so the
	                                                  // INT0-interrupt won't fire

	set_sleep_mode(SLEEP_MODE_IDLE);                  // idle - mode
	sleep_mode();                                     // enter sleep mode

}

/*
 * Boot the device (set start-configuration)
 */
void boot() {

	cli();
	uart_init();
	stdout = &uart_output;
	stdin  = &uart_input;
	sei();                      // enable global interrupts

	printf("boot\n");

	// disable power indicator on PB2
	set_bit(DDRB, PB2);
	clear_bit(PORTB, PB2);

	// configure button port on PB1 for input (pull-up enabled)
	clear_bit(DDRB, PB1);
	set_bit(PORTB, PB1);
	set_bit(portBHistory, PB1);

	// enable bit change interrupt for PB1 and PB2
	set_bit(PCMSK0, PCINT1);                          // for PB1
	set_bit(PCMSK0, PCINT4);                          // and PB4

}

/*
 * Shutdown
 */
void shutdown() {

}

void handle_buttonReleased() {

	clear_bit(PORTB, PB2);

}

void handle_buttonPressed() {

	set_bit(PORTB, PB2);

}

/*
 * Main-routine
 */
int main() {

	// configure MCU and start timers
	boot();

	while(1) {

		go_asleep();

		switch (event) {

		case BUTTON_PRESSED:
			handle_buttonPressed();
			break;

		case BUTTON_RELEASED:
			handle_buttonReleased();
			break;

		}

	}

	shutdown();

	return 0;

}
