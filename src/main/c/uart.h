// see http://www.appelsiini.net/2011/simple-usart-with-avr-libc
#ifndef UART_H
#define UART_H

#include <stdio.h>
#include <avr/io.h>
#include <stdbool.h>

extern bool UART_ACTIVE;

void uart_putchar(char c, FILE *stream);
char uart_getchar(FILE *stream);

void uart_init(void);
void uart_shutdown(void);

/* http://www.ermicro.com/blog/?p=325 */

FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);

#endif // UART_H
