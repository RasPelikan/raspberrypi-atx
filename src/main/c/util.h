#ifndef UTIL_H
#define UTIL_H

#define _CONCAT(a,b) a##b
#define CONCAT(a,b) _CONCAT(a,b)

#define clear_bit(WHERE, WHICH) WHERE &= ~(_BV(WHICH))

#define set_bit(WHERE, WHICH) WHERE |= _BV(WHICH)

#define is_bit_high(WHERE, WHICH) WHERE & _BV(WHICH)

#define is_bit_low(WHERE, WHICH) !(is_bit_high(WHERE, WHICH))

#define nop() asm volatile("nop")

typedef void (*timer_command)();

timer_command command_waiting();
void wait_n_seconds(float seconds, timer_command cmd);
void reset_n_seconds(float seconds, timer_command cmd);
void reset_timer();
void process_timer();

#endif // UTIL_H
