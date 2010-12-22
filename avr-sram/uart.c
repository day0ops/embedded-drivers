/*
 * Polling UART implementation based on ATMega32 datasheet
 * Author: Vanya A. Sergeev - <vsergeev@san.rr.com>
 * Date: 02/15/05
 */

#include "uart.h"

void UART_init(unsigned int baudRate) {
	/* Set higher byte of the computed baudrate */
	UART_BAUDH = (unsigned char)(baudRate >> 8);
	/* Set lower byte of the computed baudrate */
	UART_BAUDL = (unsigned char)baudRate;
	/* Enable UART Send and Receive */
	UART_CTRL = ((1<<RXEN)|(1<<TXEN));
	/* Set the frame format to 8 data bits, 1 stop bit, no parity */
	UART_SETT = (1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0);
}

unsigned char UART_getc(void) {
	/* Wait for receive buffer to clear */
	while ( !(UART_STAT & (1<<RXC)) )
		;
	/* Read and return the data */
	return UART_DATA;
}

void UART_putc(unsigned char data) {
	/* Wait for send buffer to clear */
	while ( !(UART_STAT & (1<<UDRE)) )
		;
	/* Send data */
	UART_DATA = data;
}

void UART_puts(char *data) {
	/* Keep on sending data until we encounter a null byte */
	while (*data != '\0')
		UART_putc(*data++);
}

void UART_flush(void) {
	unsigned char temp;
	/* While data exists, read it from the UDR register */
	while (UART_STAT & (1<<RXC))
		temp = UART_DATA;
}

