/*
 * Polling UART implementation based on ATMega32 datasheet
 * Author: Vanya A. Sergeev - <vsergeev@san.rr.com>
 * Date: 02/15/05
 */

#include <avr/io.h>

#define UART_STAT   	UCSRA
#define UART_CTRL	UCSRB
#define UART_SETT	UCSRC
#define UART_DATA     	UDR
#define	UART_BAUDH	UBRRH
#define	UART_BAUDL	UBRRL

#define	calcBaudRate(baudRate) ((unsigned char)((F_CPU/(16L * (unsigned long)baudRate)) - 1))

void UART_init(unsigned int baudRate);
unsigned char UART_getc(void);
void UART_putc(unsigned char data);
void UART_puts(char *data);
void UART_flush(void);

