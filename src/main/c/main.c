#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdint.h>

#include "util.h"
#include "uart.h"

#define ATX_PIN PB0
#define BUTTON_PIN PB1
#define BUTTON_INTERRUPT PCINT1
#define LED_PIN PB2
#define SHUTDOWN_PIN PB3
#define RASPBERRYPI_OFF_PIN PB4
#define RASPBERRYPI_OFF_INTERRUPT PCINT4

#define NO_EVENT 0
#define BUTTON_PRESSED 1
#define RASPBERRYPI_ON 2
#define RASPBERRYPI_OFF 3

#define STATE_OFF 0
#define STATE_ON 1
#define STATE_BOOTING 2
#define STATE_SHUTDOWN 3

static volatile uint8_t portBHistory = 0x00;
static char event = NO_EVENT;
static char state = STATE_OFF;

/*
 * TIMER0-interrupt - used for "wait_n_seconds"-functionality
 */
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

	if (is_bit_high(changedBits, BUTTON_PIN)) {

		if (is_bit_low(currentPortB, BUTTON_PIN)) {
			event = BUTTON_PRESSED;
		}

	}

	if (is_bit_high(changedBits, RASPBERRYPI_OFF_PIN)) {

		if (is_bit_high(currentPortB, RASPBERRYPI_OFF_PIN)) {
			event = RASPBERRYPI_ON;
		} else {
			event = RASPBERRYPI_OFF;
		}

	}

}

/*
 * bring MCU into hibernate
 */
static char go_asleep() {

	if (event != NO_EVENT) {
		return event;
	}

	/*
	 * initialize "wake up on pin-change" used for "button"- and "Raspberry Pi off"-events
	 */
	set_bit(PCICR, PCIE0);                            // enable pin change interrupt
	sei();                                            // enable interrupt since this method might
	                                                  // be called within an interrupt and if so the
	                                                  // PCINT0-interrupt won't fire

	set_sleep_mode(SLEEP_MODE_IDLE);                  // idle - mode
	sleep_mode();                                     // enter sleep mode

	char wakeUpEvent = event;
	event = NO_EVENT;                                 // reset event
	return wakeUpEvent;                               // return wake up event

}

/*
 * Boot the device (set start-configuration)
 */
void boot() {

	// uart
	cli();
	uart_init();
	stdout = &uart_output;
	stdin  = &uart_input;

	set_bit(DDRB, ATX_PIN);                           // ATX_PIN for output
	clear_bit(PORTB, ATX_PIN);                        // disable power on ATX_PIN

	set_bit(DDRB, LED_PIN);                           // LED_PIN for output
	clear_bit(PORTB, LED_PIN);                        // disable power indicator on LED_PIN

	clear_bit(DDRB, BUTTON_PIN);                      // configure BUTTON_PIN for input
	set_bit(PORTB, BUTTON_PIN);                       // pull-up enabled
	set_bit(portBHistory, BUTTON_PIN);                // used for detecting pin change events

	clear_bit(DDRB, RASPBERRYPI_OFF_PIN);             // configure RASPBERRYPI_OFF_PIN for input
	clear_bit(PORTB, RASPBERRYPI_OFF_PIN);            // pull-down enabled

	set_bit(DDRB, SHUTDOWN_PIN);                      // configure SHUTDOWN_PIN for output
	clear_bit(PORTB, SHUTDOWN_PIN);                   // no shutdown

	// enable bit change interrupt
	set_bit(PCMSK0, BUTTON_INTERRUPT);                // for BUTTON_PIN
	set_bit(PCMSK0, RASPBERRYPI_OFF_INTERRUPT);       // and RASPBERRYPI_OFF_PIN

	sei();                                            // enable global interrupts
	printf("Initialized\n");

}

void handle_raspberryPiOn() {

	state = STATE_ON;
	printf("On\n");

}

void handle_raspberryPiOff() {

	state = STATE_OFF;
	clear_bit(PORTB, ATX_PIN);
	clear_bit(PORTB, LED_PIN);
	printf("Off\n");
}

void handle_buttonPressedLong() {

	// ignore short button activity
	if (is_bit_high(PINB, BUTTON_PIN)) {
		printf("Short\n");
		return;
	}

	printf("Long\n");
	handle_raspberryPiOff();

}

void handle_buttonPressed() {

	// ignore short button activity
	if (is_bit_high(PINB, BUTTON_PIN)) {
		printf("Short\n");
		return;
	}

	if (state != STATE_OFF) {
		printf("Running\n");
		// wait 4.5 seconds for immediate shutdown
		wait_n_seconds(4.5, handle_buttonPressedLong);

	} else {

		if (is_bit_high(PINB, RASPBERRYPI_OFF_PIN)) {

			printf("Shutdown\n");
			state = STATE_SHUTDOWN;
			set_bit(PORTB, SHUTDOWN_PIN);

		} else {

			printf("Booting\n");
			state = STATE_BOOTING;
			set_bit(PORTB, ATX_PIN);
			set_bit(PORTB, LED_PIN);

		}

	}

}

/*
 * Main-routine
 */
int main() {

	boot();                                           // configure MCU

	while(1) {                                        // main loop

		char wakeUpEvent = go_asleep();               // sleep until an event occurs

		switch (wakeUpEvent) {

		case BUTTON_PRESSED:                          // handle "button pressed"-event
			wait_n_seconds(0.5, handle_buttonPressed);// ignore short button activity
			break;

		case RASPBERRYPI_ON:                          // handle "RaspberryPi on"-event
			handle_raspberryPiOn();
			break;

		case RASPBERRYPI_OFF:                         // handle "RaspberryPi off"-event
			handle_raspberryPiOff();
			break;

		}

	}

	return 0;

}
