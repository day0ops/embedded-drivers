/* 
 * ATMega32 at 16MHz interfacing with the Cypress CY62256 SRAM chip.
 * See sram.h for connection details.
 * See the data sheet for the write cycle timing diagrams.
 *
 * Date: 11/12/2007
 * Vanya A. Sergeev - <vsergeev@gmail.com>
 */

#include <avr/io.h>
#include "uart.h"
#include "sram.h"

/* SRAM_Init: sets up I/O for interfacing with the SRAM chip. */
void SRAM_Init(void) {
	/* Set up data direction registers */
	
	SRAM_CTRL_DDR |= ((1<<SRAM_CTRL_CE)|(1<<SRAM_CTRL_OE)|(1<<SRAM_CTRL_WE));
	/* Deselect the chip as soon as possible */
	SRAM_Deselect();
	SRAM_WE_High();
	SRAM_OE_High();

	SRAM_ADDRL_DDR = 0xFF;
	SRAM_ADDRH_DDR = 0x7F; // Top most address line is A14
}

/* SRAM_Write: writes 8-bit data to the 15-bit address (16th bit is ignored). */
void SRAM_Write(u16 address, u8 data) {
	/* Write Cycle No. 1 (/WE Controlled) */

	/* Setup for data output */
	SRAM_DATA_DDR = 0xFF;
	
	/* Put the address lines on their bus, ignoring the 16th bit snce the
 	 * chip can only be addressed up to 15 bits */
	SRAM_ADDRL_PORT = (u8)(address & 0x00FF);
	SRAM_ADDRH_PORT = (u8)((address & 0x7F00)>>8);

	/* Begin write cycle */
	SRAM_WE_High();
	SRAM_OE_High();
	SRAM_Select();

	SRAM_DATA_PORT = (u8)data;
	SRAM_WE_Low();
	SRAM_WE_High();

	/* End the write cycle */
	SRAM_Deselect();
}

/* SRAM_Read: reads 8-bit data from the 15-bit address (16th bit is ignored). */
u8 SRAM_Read(u16 address) {
	u8 data;

	/* Read Cycle No. 2 (WE=high) */

	/* Setup for data input */
	SRAM_DATA_DDR = 0x00;
	
	/* Put the address lines on their bus, ignoring the 16th bit snce the
 	 * chip can only be addressed up to 15 bits */
	SRAM_ADDRL_PORT = (u8)(address & 0x00FF);
	SRAM_ADDRH_PORT = (u8)((address & 0x7F00)>>8);

	/* Begin read cycle */
	SRAM_WE_High();
	SRAM_OE_High();
	SRAM_Select();

	SRAM_OE_Low();
	data = SRAM_DATA_PIN;
	SRAM_OE_High();

	/* End read cycle */
	SRAM_Deselect();

	return data;
}

int main(void) {
	u16 i;

	UART_init(calcBaudRate(9600));	

	SRAM_Init();
	
	/* Write the least significant byte of every address to the address,
 	 * if the test fails, indicate it failed and break the loop. */
	for (i = 0; i <= 0x7FFF; i++) {
		SRAM_Write(i, (u8)(i&0x00FF));
		if (SRAM_Read(i) != (u8)(i&0x00FF)) {
			UART_puts("Test failed!\r\n");
			break;
		}
	}

	/* Indicate the was successful if every memory location was written to
 	 * and read from correctly. */
	if (i == 0x8000) {
		UART_puts("Test passed!\r\n");
	}
	
	while (1) {

	}

	return 0;
}

