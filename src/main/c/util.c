#include <stddef.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "util.h"

static volatile timer_command command;                // command executed after n seconds
static int number_of_interrupts;                      // number of interrupts necessary to wait n seconds

/*
 * disable the timer used by "wait_n_seconds"
 */
void reset_timer() {

	clear_bit(TIMSK0, OCIE0A);                        // disable timer interrupt
	command = NULL;                                   // delete command store to be executed after timeout

}

timer_command command_waiting() {

	return command;

}

/*
 * based on F_CPU = 8MHz and a prescaler of 1024 the timer0 has to count up to 252
 * for 31 times since one second is elapsed
 */
void wait_n_seconds(float seconds, timer_command cmd) {

	if (seconds == 0) {                               // stop running a running timer

		reset_timer();                                // reset timer
		if (cmd != NULL) {                            // if abort-command is given
			(*cmd)();                                 // then execute it
		}
		return;                                       // leave and don't start a new timer

	}

	// store information for timer-interrupt
	command = cmd;
	number_of_interrupts = seconds * 31;

	// enable timer
	OCR0A = 252;                                      // 31 times of compare matches at 252 is 1 second
	TCNT0 = 0;                                        // start at counter 0
	TCCR0A |= _BV(WGM01);                             // compare-match mode
	TCCR0B |= _BV(CS00) | _BV(CS02);                  // prescaler 1024
	TIMSK0 |= _BV(OCIE0A);                            // use OCR0A for a compare-match

}

/*
 * resets time interval if given command is the command currently processed
 */
void reset_n_seconds(float seconds, timer_command cmd) {

	if (cmd == command) {                             // if stored command is given command

		number_of_interrupts = seconds * 31;          // then reset counter

	}

}

/*
 * "wait_n_seconds" timer-interrupt
 */
void process_timer() {

	// decrease counter and check wether time-period has elapsed
	if (--number_of_interrupts == 0) {

		void (*cmd)() = command;                      // save command
		reset_timer();                                // reset timer
		(*cmd)();                                     // run command

	}

}
