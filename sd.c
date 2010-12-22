/* SD/SPI driver for LPC2109 
 *
 * Vanya A. Sergeev - <vsergeev@gmail.com> - copyright 2010
 * please inform author of possible use, licensing is still being decided
 *
 */

#include "sd.h"

/* A few public ariables keeping track of the SD's capacity
 * and block length. */
int sd_high_capacity;
int sd_block_len;
int sd_mmc;
uint8_t sd_csd[SD_CSD_LENGTH];
uint8_t sd_cid[SD_CID_LENGTH];

#ifdef SD_DEBUG
#include "debug.h"
#endif

#ifdef SD_DEBUG

void sd_debug_print_boolean(uint8_t data) {
	if (data == 1)
		debug_printf("true");
	else if (data == 0)
		debug_printf("false");
}

void sd_debug_print_data_block(uint8_t *data) {
	int i;

	for (i = 0; i < sd_block_len; i++) {
		debug_printf("%02X ", data[i]);
		if (((i+1) % 16) == 0) {
			debug_printf("\n");
		} else if (((i+1) % 8) == 0) {
			debug_printf(" ");
		}
	}
	debug_printf("\n");
}

void sd_debug_print_csd(void) {
	int csd_version, i;
	uint8_t temp;

	debug_printf("\n");
	debug_printf("     ********************************\n");
	debug_printf("     * CSD\n");
	debug_printf("     *\n");
	debug_printf("     * Raw CSD Bytes: ");
	for (i = 0; i < 16; i++)
		debug_printf("%02X ", sd_csd[i]);
	debug_printf("\n");

	debug_printf("     *\n");

	csd_version = (sd_csd[0] & 0xC0) >> 6;

	if (csd_version == 0)
		debug_printf("     * VERSION: 1.0\n");
	else if (csd_version == 1)
		debug_printf("     * VERSION: 2.0\n");
	else
		debug_printf("     * VERSION: Unknown\n");

	debug_printf("     * TAAC: %X\n", sd_csd[1]);
	debug_printf("     * NSAC: %X\n", sd_csd[2]);
	debug_printf("     * TRAN_SPEED: %X\n", sd_csd[3]);

	// CCC goes here

	debug_printf("     * READ_BL_LEN: %X\n", (sd_csd[5] & 0x0F));
	debug_printf("     * READ_BL_PARTIAL: ");
	sd_debug_print_boolean((sd_csd[6] & 0x80)>>7);
	debug_printf("\n");

	debug_printf("     * WRITE_BLK_MISALIGN: ");
	sd_debug_print_boolean((sd_csd[6] & 0x40)>>6);
	debug_printf("\n");

	debug_printf("     * READ_BLK_MISALIGN: ");
	sd_debug_print_boolean((sd_csd[6] & 0x20)>>5);
	debug_printf("\n");

	debug_printf("     * DSR_IMP: ");
	sd_debug_print_boolean((sd_csd[6] & 0x10)>>4);
	debug_printf("\n");
	
	if (csd_version != 1) {	
		temp = (sd_csd[6] & 0x3) << 2;
		temp |= ((sd_csd[7] & 0xC0) >> 6);
		debug_printf("     * C_SIZE: %0X ", temp);
		temp = (sd_csd[7] & 0x3F) << 2;
		temp |= ((sd_csd[8] & 0xC0) >> 6);
		debug_printf("%0X\n", temp);

		debug_printf("     * VDD_R_CURR_MIN: %X\n", ((sd_csd[8] & 0x38) >> 3));
		debug_printf("     * VDD_R_CURR_MAX: %X\n", (sd_csd[8] & 0x7));
		debug_printf("     * VDD_W_CURR_MIN: %X\n", (sd_csd[9] & 0xE0) >> 5);
		debug_printf("     * VDD_W_CURR_MAX: %X\n", ((sd_csd[9] & 0x1C) >> 2));

		temp = (sd_csd[9] & 0x3) << 1;
		temp |= ((sd_csd[10] & 0x80) >> 7);
		debug_printf("     * C_SIZE_MULT: %X\n", temp);
	} else {
		debug_printf("     * C_SIZE: %0X %0X %0X\n", (sd_csd[7] & 0x3F), sd_csd[8], sd_csd[9]);
	}

	debug_printf("     * ERASE_BLK_EN: ");
	sd_debug_print_boolean((sd_csd[10] & 0x40) >> 6);
	debug_printf("\n");

	temp = (sd_csd[10] & 0x3F) << 1;
	temp |= (sd_csd[11] & 0x80) >> 7;
	debug_printf("     * SECTOR_SIZE: %0X\n", temp);

	debug_printf("     * WP_GRP_SIZE: %X\n", (sd_csd[11] & 0x7F));

	debug_printf("     * WP_GRP_ENABLE: ");
	sd_debug_print_boolean((sd_csd[12] & 0x80) >> 7);
	debug_printf("\n");

	debug_printf("     * R2W_FACTOR: %X\n", (sd_csd[12] & 0x1C) >> 2);

	temp = (sd_csd[12] & 0x3) << 2;
	temp |= ((sd_csd[13] & 0xC0) >> 6);
	debug_printf("     * WRITE_BL_LEN: %X\n", temp);

	debug_printf("     * WRITE_BL_PARTIAL: ");
	sd_debug_print_boolean((sd_csd[13] & 0x20) >> 5);
	debug_printf("\n");

	debug_printf("     * FILE_FORMAT_GRP: ");
	sd_debug_print_boolean((sd_csd[14] & 0x80) >> 7);
	debug_printf("\n");

	debug_printf("     * COPY: ");
	sd_debug_print_boolean((sd_csd[14] & 0x40) >> 6);
	debug_printf("\n");

	debug_printf("     * PERM_WRITE_PROTECT: ");
	sd_debug_print_boolean((sd_csd[14] & 0x20) >> 5);
	debug_printf("\n");

	debug_printf("     * TMP_WRITE_PROTECT: ");
	sd_debug_print_boolean((sd_csd[14] & 0x10) >> 4);
	debug_printf("\n");

	debug_printf("     * FILE_FORMAT: %X\n", (sd_csd[14] & 0x0C) >> 2);
	
	debug_printf("     *\n");
	debug_printf("     ********************************\n\n");
}

void sd_debug_print_cid(void) {
	int i;

	debug_printf("\n");
	debug_printf("     ********************************\n");
	debug_printf("     * CID\n");
	debug_printf("     *\n");
	debug_printf("     * Raw CID Bytes: ");
	for (i = 0; i < 16; i++)
		debug_printf("%X ", sd_cid[i]);
	debug_printf("\n");
	debug_printf("     *\n");

	debug_printf("     * Manufacturer ID (MID): %X\n", sd_cid[0]);
	debug_printf("     * OEM/Application ID (OID): %c%c\n", sd_cid[1], sd_cid[2]);
	debug_printf("     * Product name (PNM): %c%c%c%c%c%c\n", sd_cid[3], sd_cid[4], sd_cid[5], sd_cid[6], sd_cid[7]);
	debug_printf("     * Product revision (PRV): %X\n", sd_cid[8]);
	debug_printf("     * Product serial number (PSV): %X%X%X%X\n", sd_cid[9], sd_cid[10], sd_cid[11], sd_cid[12]);
	debug_printf("     * Manufacturing date (MDT): %X %X\n", (sd_cid[13] & 0x0F), sd_cid[14]);
	debug_printf("     *\n");
	debug_printf("     ********************************\n\n");
}

#endif

void sd_debug_print(char *message, uint8_t *data, int dataLen) {
	#ifdef SD_DEBUG
	int i;

	debug_printf(message);
	for (i = 0; i < dataLen; i++)
		debug_printf("%02X", data[i]);
	debug_printf("\n");	
	#endif		
}

/******************************************************************************
 *** Low-level SPI interface functions                                      ***
 ******************************************************************************/

void sd_cd_wp_init(void) {
	/* Set card detect and write protect pins at inputs */
	FIO0DIR &= ~((1<<18)|(1<<19));
}

uint8_t sd_card_detect(void) {
	/* Return the status of the card detect switch */
	if (FIO0PIN & (1<<18))
		return 1;
	return 0;
}

uint8_t sd_write_protect(void) {
	/* Return the status of the write protect switch */
	if (FIO0PIN & (1<<19))
		return 1;
	return 0;
}

void sd_spi_init(void) {
	int i;

	/* Enable SCK0, MISO0, and MOSI0 for SPI0 bus use,
	 * but declare SSEL0 as a GPIO for now. */
	PINSEL0 |= ((1<<8)|(1<<10)|(1<<12));
	PINSEL0 &= ~((1<<9)|(1<<11)|(1<<13)|(1<<15)|(1<<14));

	/* Set the SPI Clock speed to 400KHz for now (initialization)
	 * PCLK = 60MHz, SPI Rate = 400KHz = 60/150, S0SPCCR = 150 */
	S0SPCCR = 150;

	/* Set the SPI Control Register
	 * 8 bits data, CPOL = 0, CPHA = 0, Master = 1,
	 * LSBF = 0 (MSB first), SPIE = 0 */
	S0SPCR = (1<<5);
	
	/* Set the SD chip select pin as an output */
	SD_CS_IODIR |= SD_CS_PIN;

	/* Hold SSEL0 high so the SD card can initialize itself with
	 * the next 80 clocks on SCK0. */
	sd_spi_deselect();
	
	/* Clock out at least 74 cycles with no data, so the SD card can 
	 * initialize itself. */
	for (i = 0; i < 15; i++)
		sd_spi_delay_clocks();
}

void sd_spi_send(uint8_t data) {
	volatile uint8_t dummy;

	/* Put the data in the shift register */
	S0SPDR = data;
	/* Wait until the transfer complete flag clears */
	while ((S0SPSR & (1<<7)) != (1<<7))
		;
	/* Read S0SPDR to clear the status register */
	dummy = S0SPDR;
}

uint8_t sd_spi_receive(void) {
	sd_spi_select();
	/* Send a dummy byte */
	S0SPDR = 0xFF;
	/* Wait until the transfer complete flag clears */
	while ((S0SPSR & (1<<7)) != (1<<7))
		;
	sd_spi_deselect();
	/* Read the data clocked in */
	return S0SPDR;
}

void sd_spi_delay_clocks(void) {
	/* Ensure that CS is high */
	sd_spi_select();
	/* Send a dummy byte */
	sd_spi_send(0xFF);
}

/******************************************************************************
 *** SD CRC and Command Functions                                           ***
 ******************************************************************************/

uint16_t sd_crc16_bits(uint8_t data, uint16_t seed) {
	int i, feedback;

	/*                                            Feedback
	 *   --------------------------------------------------------------------------------------X--Input
	 *   |                       |                                       |                     |
	 *  [0]->[1]->[2]->[3]->[4]->X->[5]->[6]->[7]->[8]->[9]->[10]->[11]->X->[12]->[13]->[14]->[15]
	 *
	 *   0...15 = seed
	 *   X = XOR
	 *
	 */

	for (i = 0; i < 8; i++) {
		/* Feedback from xor of input and seed MSB bits */
		feedback = ((data>>7) ^ (seed>>15)) & 0x1;
		/* If we have no feedback, we have nothing to XOR
		 * and we shift the seed normally to the left */
		if (feedback == 0) {
			seed <<= 1;
		} else {
			/* Otherwise, XOR the feedback bits onto the seed */
			seed ^= (0x10|0x800);
			/* Shift the seed */
			seed <<= 1;
			/* Append a one to the bottom of the seed */
			seed |= 0x01;
		}
		/* Shift the data to the left */
		data <<= 1;
	}

	return seed;
}

uint16_t sd_crc16_data(const uint8_t *data, int dataLen) {
	int i;
	uint16_t seed = 0;

	for (i = 0; i < dataLen; i++) {
		seed = sd_crc16_bits(data[i], seed);
	}

	return seed;
}

uint8_t sd_crc7_bits(uint8_t data, uint8_t seed) {
	int i, feedback;

	/*              Feedback
	 *   ---------------------------------X--Input
	 *   |             |                  |
	 *  [0]->[1]->[2]->X->[3]->[4]->[5]->[6]
	 *
	 *   0...6 = seed
	 *   X = XOR
	 *
	 */

	for (i = 0; i < 8; i++) {
		/* Feedback from xor of input and seed MSB bits */
		feedback = (((data&0x80)>>7) ^ ((seed&0x40)>>6)) & 0x1;
		/* If we have no feedback, we have nothing to XOR
		 * and we shift the seed normally to the left */
		if (feedback == 0) {
			seed <<= 1;
		} else {
			/* Otherwise, XOR the feedback bit onto the seed */
			seed ^= 0x04;
			/* Shift the seed */
			seed <<= 1;
			/* Append a one to the bottom of the seed */
			seed |= 0x01;
		}
		/* Shift the data to the left */
		data <<= 1;
	}

	return (seed & 0x7F);
}

uint8_t sd_crc7_packet(const uint8_t *data, int dataLen) {
	int i;
	uint8_t seed = 0;

	for (i = 0; i < dataLen; i++)	
		seed = sd_crc7_bits(data[i], seed);

	/* Append the end 1 bit */
	seed <<= 1;
	seed |= 0x1;	

	return seed;
}

void sd_spi_command_send(uint8_t command, uint32_t argument) {
	uint8_t packet[6];
	int i;

	sd_spi_select();

	/* Begin to transmit the 6-byte packet:
	 * 1 byte command, 4 bytes argument, 1 byte CRC */

	/* Encode 01cc cccc, where c is the 6-bit command */
	packet[0] = 0x40 | (command&0x3F);
	/* Encode most significant to least significant argument bytes */
	packet[1] = (uint8_t)((argument>>24)&0xFF);
	packet[2] = (uint8_t)((argument>>16)&0xFF);
	packet[3] = (uint8_t)((argument>>8)&0xFF);
	packet[4] = (uint8_t)(argument&0xFF);
	/* Calculate the CRC7 */
	packet[5] = sd_crc7_packet(packet, 5);

	for (i = 0; i < 6; i++)
		sd_spi_send(packet[i]);
	
	sd_spi_deselect();
}

void sd_spi_command_response(uint8_t *response, int responseLength) {
	int i;

	/* Read the response */

	/* Wait until we start getting some data..*/
	for (i = 0; i < SD_SPI_CMD_READ_ATTEMPTS; i++) {
		response[0] = sd_spi_receive();
		if (response[0] != 0xFF)
			break;
	}
	for (i = 0; i < (responseLength-1); i++) {
		response[i+1] = sd_spi_receive();
	}
}
	
void sd_spi_command(uint8_t command, uint32_t argument, int responseLength, uint8_t *response) {
	sd_spi_command_send(command, argument);
	sd_spi_command_response(response, responseLength);
	sd_spi_delay_clocks();
}

/******************************************************************************
 *** Higher-level command interface functions                               ***
 ******************************************************************************/

int sd_read_csd(void) {
	uint8_t response[1];
	uint16_t crc16;
	int i;

	/* Send the CSD command and receive the R1 response. */
	sd_spi_command_send(SD_CMD9, 0x00);
	sd_spi_command_response(response, SD_CMD9_RL);

	/* We have received R1, next up is the CSD and CRC
	 * data: */

	/* Find the data block start byte */
	for (i = 0; i < SD_SPI_DATA_READ_ATTEMPTS; i++) {
		sd_csd[0] = sd_spi_receive();
		if (sd_csd[0] == SD_SPI_DATA_BLOCK_START)
			break;
	}
	/* Receive the 16-byte CSD */
	for (i = 0; i < SD_CSD_LENGTH; i++)
		sd_csd[i] = sd_spi_receive();

	/* Receive the CRC16 of the CSD */
	crc16 = (sd_spi_receive() << 8);
	crc16 |= sd_spi_receive();
	
	sd_spi_delay_clocks();

	/* Check the R1 response for errors */
	if (response[0] != 0x00) {
		sd_debug_print("* SD -- Failure: CMD9. Error receiving CSD. Response: ", response, SD_CMD9_RL);
		return SD_ERROR_GET_CSD;
	}

	/* Verify the CSD's data CRC */
	if (sd_crc16_data(sd_csd, 16) != crc16) {
		sd_debug_print("* SD -- Failure: CMD9. CRC16 invalid on CSD data.", 0, 0);
		return SD_ERROR_GET_CSD_CRC;
	}
	
	sd_debug_print("* SD -- Success: CMD9. Retrieved card CSD.", 0, 0);
	return 0;
}

int sd_read_cid(void) {
	uint8_t response[1];
	uint16_t crc16;
	int i;

	/* Send the CID command and receive the R1 response. */
	sd_spi_command_send(SD_CMD10, 0x00);
	sd_spi_command_response(response, SD_CMD10_RL);

	/* We have received R1, next up is the CID and CRC
	 * data: */

	/* Find the data block start byte */
	for (i = 0; i < SD_SPI_DATA_READ_ATTEMPTS; i++) {
		sd_cid[0] = sd_spi_receive();
		if (sd_cid[0] == SD_SPI_DATA_BLOCK_START)
			break;
	}
	/* Receive the 16-byte CID */
	for (i = 0; i < SD_CID_LENGTH; i++)
		sd_cid[i] = sd_spi_receive();

	/* Receive the CRC16 of the CID */
	crc16 = (sd_spi_receive() << 8);
	crc16 |= sd_spi_receive();
	
	sd_spi_delay_clocks();

	/* Check the R1 response for errors */
	if (response[0] != 0x00) {
		sd_debug_print("* SD -- Failure: CMD10. Error receiving CID. Response: ", response, SD_CMD9_RL);
		return SD_ERROR_GET_CID;
	}

	/* Verify the CID's data CRC */
	if (sd_crc16_data(sd_cid, 16) != crc16) {
		sd_debug_print("* SD -- Failure: CMD10. CRC16 invalid on CID data.", 0, 0);
		return SD_ERROR_GET_CID_CRC;
	}

	sd_debug_print("* SD -- Success: CMD10. Retrieved card CID.", 0, 0);
	return 0;
}

int sd_erase_blocks(uint32_t address_start, uint32_t address_end) {
	uint8_t response[5];

	/* Note: SD uses CMD32 and CMD33 to define the erase region,
	 * whereas MMC uses CMD35 and CMD36 to define the erase region. */

	/* Send the erase address start command with our start address */
	if (!sd_mmc) sd_spi_command(SD_CMD32, address_start, SD_CMD32_RL, response);
		else sd_spi_command(SD_CMD35, address_start, SD_CMD35_RL, response);
	if (response[0] != 0x00) {
		if (response[0] & 0x40) {
			if (!sd_mmc) sd_debug_print("* SD -- Failure: CMD32. Erase start address misaligned. Response: ", response, SD_CMD32_RL);
				else sd_debug_print("* SD -- Failure: CMD35. Erase start address misaligned. Response: ", response, SD_CMD35_RL);

			return SD_ERROR_START_ADDR_MISALIGNED;
		}
		if (response[0] & 0x80) {
			if (!sd_mmc) sd_debug_print("* SD -- Failure: CMD32. Erase start address out of bounds. Response: ", response, SD_CMD32_RL);
				else sd_debug_print("* SD -- Failure: CMD35. Erase start address out of bounds. Response: ", response, SD_CMD35_RL);

			return SD_ERROR_START_ADDR_OUTBOUNDS;
		}
		if (!sd_mmc) sd_debug_print("* SD -- Failure: CMD32. Unknown error setting erase start address. Response: ", response, SD_CMD32_RL);
			else sd_debug_print("* SD -- Failure: CMD35. Unknown error setting erase start address. Response: ", response, SD_CMD35_RL);
		return SD_ERROR_START_ADDR_UNKNOWN;
	}
	if (!sd_mmc) sd_debug_print("* SD -- Success: CMD32. Erase start address set.", 0, 0);
		else sd_debug_print("* SD -- Success: CMD35. Erase start address set.", 0, 0);
	
	/* Send the erase address end command with our end address */
	if (!sd_mmc)
		sd_spi_command(SD_CMD33, address_end, SD_CMD33_RL, response);
	else
		sd_spi_command(SD_CMD36, address_end, SD_CMD36_RL, response);
	if (response[0] != 0x00) {
		if (response[0] & 0x40) {
			if (!sd_mmc) sd_debug_print("* SD -- Failure: CMD33. Erase end address misaligned. Response: ", response, SD_CMD33_RL);
				else sd_debug_print("* SD -- Failure: CMD36. Erase end address misaligned. Response: ", response, SD_CMD36_RL);
			return SD_ERROR_END_ADDR_MISALIGNED;
		}
		if (response[0] & 0x80) {
			if (!sd_mmc) sd_debug_print("* SD -- Failure: CMD33. Erase end address out of bounds. Response: ", response, SD_CMD33_RL);
				else sd_debug_print("* SD -- Failure: CMD36. Erase end address out of bounds. Response: ", response, SD_CMD36_RL);
			return SD_ERROR_END_ADDR_OUTBOUNDS;
		}
		if (!sd_mmc) sd_debug_print("* SD -- Failure: CMD33. Unknown error setting erase end address. Response: ", response, SD_CMD33_RL);
			else sd_debug_print("* SD -- Failure: CMD36. Unknown error setting erase end address. Response: ", response, SD_CMD36_RL);
		return SD_ERROR_END_ADDR_UNKNOWN;
	}
	if (!sd_mmc) sd_debug_print("* SD -- Success: CMD33. Erase end address set.", 0, 0);
		else sd_debug_print("* SD -- Success: CMD36. Erase end address set.", 0, 0);

	/* Send the erase command */
	sd_spi_command_send(SD_CMD38, 0x00);
	sd_spi_command_response(response, SD_CMD38_RL);
	if (response[0] != 0x00) {
		sd_debug_print("* SD -- Failure: CMD38. Error erasing selected blocks. Response: ", response, SD_CMD38_RL);
		return SD_ERROR_ERASE;
	}

	/* Wait for the busy signal to clear */
	while (1) {
		if (sd_spi_receive() != SD_SPI_BUSY)
			break;
	}
	
	sd_spi_delay_clocks();

	sd_debug_print("* SD -- Success: CMD38. Selected blocks erased.", 0, 0);

	return 0;
}

int sd_stop_block_transmission(void) {
	uint8_t data;
	
	sd_spi_command_send(SD_CMD12, 0x00);
	/* Wait for the command to take into effect */
	sd_spi_delay_clocks();
	
	/* Check our R1 response 
	if (response[0] != 0x00) {
		sd_debug_print("* SD -- Failure: CMD12. Error stopping multiple block transmission. Response: ", response, SD_CMD12_RL);
		return SD_ERROR_STOP_MULTIPLE_BLOCKS;
	} */

	/* Now wait for the 0x00 busy signal to clear */
	for (;;) {
		data = sd_spi_receive();
		if (data == 0xFF)
			break;
	}

	sd_debug_print("* SD -- Success: CMD12. Multiple block transmission stopped.", 0, 0);
	
	return 0;
}

int sd_write_block(uint32_t address, const uint8_t *data) {
	uint8_t response[5];
	uint16_t crc16;
	int i, retVal;

	/* Error out if the address is not aligned by block length */
	if ((address % sd_block_len) != 0) {
		sd_debug_print("* SD -- Failure: CMD24. Address not aligned by block length.", 0, 0);
		return SD_ERROR_WRITE_ADDR_MISALIGNED;
	}

	/* If this is a high capacity card, the data is addressed in
	 * blocks (512 bytes). Adjust the address accordingly. */
	if (sd_high_capacity) {
		address /= sd_block_len;
	}

	/* Send the write single block command and receive the R1 response */
	sd_spi_command(SD_CMD24, address, SD_CMD24_RL, response);
	
	if (response[0] != 0x00) {
		if (response[0] & 0x40) {
			sd_debug_print("* SD -- Failure: CMD24. Write data address misaligned. Response: ", response, SD_CMD24_RL);
			return SD_ERROR_WRITE_ADDR_MISALIGNED;
		}
		if (response[0] & 0x80) {
			sd_debug_print("* SD -- Failure: CMD24. Write data address out of bounds. Response: ", response, SD_CMD24_RL);
			return SD_ERROR_WRITE_ADDR_OUTBOUNDS;
		}
		sd_debug_print("* SD -- Failure: CMD24. Unknown error with single block write. Response: ", response, SD_CMD24_RL);
		return SD_ERROR_WRITE_UNKNOWN;
	}

	/* CRC16 the data we're sending */
	crc16 = sd_crc16_data(data, sd_block_len);

	/* Send the data block start token and the data block */
	sd_spi_send(SD_SPI_DATA_BLOCK_START);

	/* Send every byte of the data block */
	for (i = 0; i < sd_block_len; i++)
		sd_spi_send(data[i]);

	/* Send the CRC16 of the data block */
	sd_spi_send((uint8_t)(crc16 >> 8));
	sd_spi_send((uint8_t)(crc16 & 0xFF));

	/* Wait for the data response token */
	for (i = 0; i < SD_SPI_DATA_READ_ATTEMPTS; i++) {
		response[0] = sd_spi_receive();
		if (response[0] != 0xFF)
			break;
	}

	/* Check the response token */	
	switch ((response[0] & 0x0E) >> 1) {
		case SD_SPI_WRITE_ACCEPTED:
			retVal = 0;
			break;
		case SD_SPI_WRITE_ERROR_CRC:
			sd_debug_print("* SD -- Failure: CMD24. CRC error occured during single block write. Data response token: ", response, 1);
			retVal = SD_ERROR_WRITE_BLOCK_CRC;
			break;
		case SD_SPI_WRITE_ERROR_WRITE:
			sd_debug_print("* SD -- Failure: CMD24. Write error occured during single block write. Data response token: ", response, 1);
			retVal = SD_ERROR_WRITE_BLOCK;
			break;
		default:
			sd_debug_print("* SD -- Failure: CMD24. Unknown error occured during single block write. Data response token: ", response, 1);
			retVal = SD_ERROR_WRITE_UNKNOWN;
			break;
	}

	/* Wait for the busy signal to clear */
	while (1) {
		if (sd_spi_receive() != SD_SPI_BUSY)
			break;
	}

	sd_spi_delay_clocks();

	if (retVal == 0)
		sd_debug_print("* SD -- Success: CMD24. Single data block written.", 0, 0); 

	return retVal; 
}

int sd_write_blocks(uint32_t address, const uint8_t *data, int dataLen) {
	uint8_t response[5];
	uint16_t crc16;
	int i, dataIndex, retVal;

	/* Error out if the address is not aligned by block length */
	if ((address % sd_block_len) != 0) {
		sd_debug_print("* SD -- Failure: CMD25. Address not aligned by block length.", 0, 0);
		return SD_ERROR_WRITE_ADDR_MISALIGNED;
	}
	
	/* Make sure the data length is in multiples of the block length. */
	if ((dataLen % sd_block_len) != 0) {
		sd_debug_print("* SD -- Failure: CMD25. Data length not in block multiples.", 0, 0);
		return SD_ERROR_WRITE_DATALEN_MULTIPLE;
	}

	/* If this is a high capacity card, the data is addressed in
	 * blocks (512 bytes). Adjust the address accordingly. */
	if (sd_high_capacity) {
		address /= sd_block_len;
	}

	/* Send the write multiple blocks command and receive the R1 response */
	sd_spi_command(SD_CMD25, address, SD_CMD25_RL, response);
	
	if (response[0] != 0x00) {
		if (response[0] & 0x40) {
			sd_debug_print("* SD -- Failure: CMD25. Write data address misaligned. Response: ", response, SD_CMD25_RL);
			return SD_ERROR_WRITE_ADDR_MISALIGNED;
		}
		if (response[0] & 0x80) {
			sd_debug_print("* SD -- Failure: CMD25. Write data address out of bounds. Response: ", response, SD_CMD25_RL);
			return SD_ERROR_WRITE_ADDR_OUTBOUNDS;
		}
		sd_debug_print("* SD -- Failure: CMD25. Unknown error with single block write. Response: ", response, SD_CMD25_RL);
		return SD_ERROR_WRITE_UNKNOWN;
	}

	for (retVal = 0, dataIndex = 0; dataIndex < dataLen; ) {
		/* CRC16 the data we're sending */
		crc16 = sd_crc16_data(data+dataIndex, sd_block_len);

		/* Send the data block start token and the data block */
		sd_spi_send(SD_SPI_MULTIPLE_DATA_BLOCK_START);

		/* Send every byte of the data block */
		for (i = 0; i < sd_block_len; i++)
			sd_spi_send(data[dataIndex++]);

		/* Send the CRC16 of the data block */
		sd_spi_send((uint8_t)(crc16 >> 8));
		sd_spi_send((uint8_t)(crc16 & 0xFF));

		/* Wait for the data response token */
		for (i = 0; i < SD_SPI_DATA_READ_ATTEMPTS; i++) {
			response[0] = sd_spi_receive();
			if (response[0] != 0xFF)
				break;
		}

		/* Check the response token */	
		switch ((response[0] & 0x0E) >> 1) {
			case SD_SPI_WRITE_ACCEPTED:
				retVal = 0;
			break;
			case SD_SPI_WRITE_ERROR_CRC:
			sd_debug_print("* SD -- Failure: CMD25. CRC error occured during multiple block write. Data response token: ", response, 1);
			retVal = SD_ERROR_WRITE_BLOCK_CRC;
			break;
			case SD_SPI_WRITE_ERROR_WRITE:
			sd_debug_print("* SD -- Failure: CMD25. Write error occured during multiple block write. Data response token: ", response, 1);
			retVal = SD_ERROR_WRITE_BLOCK;
			break;
			default:
			sd_debug_print("* SD -- Failure: CMD25. Unknown error occured during multiple block write. Data response token: ", response, 1);
			retVal = SD_ERROR_WRITE_UNKNOWN;
			break;
		}

		/* Wait for the busy signal to clear */
		while (1) {
			if (sd_spi_receive() != SD_SPI_BUSY)
				break;
		}

		sd_spi_delay_clocks();
	
		/* Check if we had any errors with this block */	
		if (retVal < 0)
			break;
	}

	/* Send Stop Transmission token */
	sd_spi_send(SD_SPI_MULTIPLE_DATA_BLOCK_END);
	sd_spi_delay_clocks();
		
	/* Wait for the busy signal to clear */
	while (1) {
		if (sd_spi_receive() != SD_SPI_BUSY)
			break;
	}

	sd_debug_print("* SD -- Success: CMD25. Multiple blocks written.", 0, 0);

	sd_spi_delay_clocks();
	
	return retVal;
}

int sd_read_blocks(uint32_t address, uint8_t *data, int dataLen) {
	uint8_t response[5];
	uint16_t crc16;
	int i, dataIndex, retVal;

	/* Align the address with the nearest block length down */
	//address -= (address % sd_block_len);

	/* Error out if the address is not aligned by block length */
	if ((address % sd_block_len) != 0) {
		sd_debug_print("* SD -- Failure: CMD18. Address not aligned by block length.", 0, 0);
		return SD_ERROR_READ_ADDR_MISALIGNED;
	}

	/* Make sure the data length is in multiples of the block length. */
	if ((dataLen % sd_block_len) != 0) {
		sd_debug_print("* SD -- Failure: CMD18. Data length not in block multiples.", 0, 0);
		return SD_ERROR_READ_DATALEN_MULTIPLE;
	}

	/* If this is a high capacity card, the data is addressed in
	 * blocks (512 bytes). Adjust the address accordingly. */
	if (sd_high_capacity) {
		address /= sd_block_len;
	}

	/* Send the read multiple blocks command and receive the R1 response */
	sd_spi_command_send(SD_CMD18, address);
	sd_spi_command_response(response, SD_CMD18_RL);

	if (response[0] != 0x00) {
		if (response[0] & 0x40) {
			sd_debug_print("* SD -- Failure: CMD18. Read data address misaligned. Response: ", response, SD_CMD18_RL);
			return SD_ERROR_READ_ADDR_MISALIGNED;
		}
		if (response[0] & 0x80) {
			sd_debug_print("* SD -- Failure: CMD18. Read data address out of bounds. Response: ", response, SD_CMD18_RL);
			return SD_ERROR_READ_ADDR_OUTBOUNDS;
		}
		sd_debug_print("* SD -- Failure: CMD18. Unknown error with multiple block read. Response: ", response, SD_CMD18_RL);
		return SD_ERROR_READ_UNKNOWN;
	}

	for (retVal = 0, dataIndex = 0; dataIndex < dataLen; ) {
		/* Find the data block start byte */
		for (i = 0; i < SD_SPI_DATA_READ_ATTEMPTS; i++) {
			data[dataIndex] = sd_spi_receive();
			/* Check if we get the start of a block or a data read error */
			if (data[dataIndex] == SD_SPI_DATA_BLOCK_START || 
					(data[dataIndex] & SD_SPI_DATA_ERROR_TOKEN_MASK) == 0x00)
				break;
		}

		if ((data[dataIndex] & SD_SPI_DATA_ERROR_TOKEN_MASK) == 0x00) {
			if (sd_mmc && (data[0] & 0x10)) {
				sd_debug_print("* SD -- Failure: CMD18. Read data address misaligned. Error token: ", data, 1);
				return SD_ERROR_READ_ADDR_MISALIGNED;
			}
			if (data[dataIndex] & 0x08) {
				sd_debug_print("* SD -- Failure: CMD18. Read data address out of range. Error token: ", data, 1);
				retVal = SD_ERROR_READ_ADDR_OUTBOUNDS;
				break;
			}
			if (data[dataIndex] & 0x04) {
				sd_debug_print("* SD -- Failure: CMD18. Card ECC failure during read. Error token: ", data, 1);
				retVal = SD_ERROR_READ_CARD_ECC;
				break;
			}
			if (data[dataIndex] & 0x02) {
				sd_debug_print("* SD -- Failure: CMD18. Card CC failure during read. Error token: ", data, 1);
				retVal = SD_ERROR_READ_CARD_CC;
				break;
			}
			sd_debug_print("* SD -- Failure: CMD18. Unknown error with multiple block read. Error token: ", data, 1);
			return SD_ERROR_READ_UNKNOWN;
		}

		/* Read a block length of data */
		for (i = 0; i < sd_block_len; i++, dataIndex++) {
			data[dataIndex] = sd_spi_receive();
		}

		/* Read in the CRC16 */
		crc16 = (sd_spi_receive() << 8);
		crc16 |= sd_spi_receive();

		/* Verify the data block's CRC */
		if (sd_crc16_data(data+(dataIndex-sd_block_len), sd_block_len) != crc16) {
			sd_debug_print("* SD -- Failure: CMD18. CRC16 invalid on read data block.", 0, 0);
			retVal = SD_ERROR_READ_MULTIPLE_CRC;
			break;
		}
		sd_debug_print("block ok", 0, 0);
	}

	/* Check if we had any block read errors */
	if (retVal < 0) {
		sd_stop_block_transmission();
		sd_spi_delay_clocks();
		return retVal;
	}
	
	/* Stop any further block transmissions */
	retVal = sd_stop_block_transmission();
	sd_spi_delay_clocks();
	/* Check if stopping block transmission went through smoothly */
	if (retVal < 0)
		return retVal;
	
	sd_debug_print("* SD -- Success: CMD18. Retrieved multiple data blocks.", 0, 0);
	return 0;
}

int sd_read_block(uint32_t address, uint8_t *data) {
	uint8_t response[5];
	uint16_t crc16;
	int i;

	/* Align the address with the nearest block length down */
	//address -= (address % sd_block_len);

	/* Error out if the address is not aligned by block length */
	if ((address % sd_block_len) != 0) {
		sd_debug_print("* SD -- Failure: CMD17. Address not aligned by block length.", 0, 0);
		return SD_ERROR_READ_ADDR_MISALIGNED;
	}

	/* If this is a high capacity card, the data is addressed in
	 * blocks (512 bytes). Adjust the address accordingly. */
	if (sd_high_capacity) {
		address /= sd_block_len;
	}

	/* Make sure we can hold up to one block 
	if (dataLen < sd_block_len) {
		sd_debug_print("* SD -- Failure: CMD17. Insufficient data length to support block size.", 0, 0);
		return SD_ERROR_READ_DATALEN;
	} */

	/* Send the read single block command and receive the R1 response */
	sd_spi_command_send(SD_CMD17, address);
	sd_spi_command_response(response, SD_CMD17_RL);

	if (response[0] != 0x00) {
		if (response[0] & 0x40) {
			sd_debug_print("* SD -- Failure: CMD17. Read data address misaligned. Response: ", response, SD_CMD17_RL);
			return SD_ERROR_READ_ADDR_MISALIGNED;
		}
		if (response[0] & 0x80) {
			sd_debug_print("* SD -- Failure: CMD17. Read data address out of bounds. Response: ", response, SD_CMD17_RL);
			return SD_ERROR_READ_ADDR_OUTBOUNDS;
		}
		sd_debug_print("* SD -- Failure: CMD17. Unknown error with single block read. Response: ", response, SD_CMD17_RL);
		return SD_ERROR_READ_UNKNOWN;
	}
	
	/* Find the data block start byte */
	for (i = 0; i < SD_SPI_DATA_READ_ATTEMPTS; i++) {
		data[0] = sd_spi_receive();
		/* Check if we get the start of a block or a data read error */
		if (data[0] == SD_SPI_DATA_BLOCK_START || 
		   (data[0] & SD_SPI_DATA_ERROR_TOKEN_MASK) == 0x00)
			break;
	}

	if ((data[0] & SD_SPI_DATA_ERROR_TOKEN_MASK) == 0x00) {
		if (sd_mmc && (data[0] & 0x10)) {
			sd_debug_print("* SD -- Failure: CMD17. Read data address misaligned. Error token: ", data, 1);
			return SD_ERROR_READ_ADDR_MISALIGNED;
		}
		if (data[0] & 0x08) {
			sd_debug_print("* SD -- Failure: CMD17. Read data address out of range. Error token: ", data, 1);
			return SD_ERROR_READ_ADDR_OUTBOUNDS;
		}
		if (data[0] & 0x04) {
			sd_debug_print("* SD -- Failure: CMD17. Card ECC failure during read. Error token: ", data, 1);
			return SD_ERROR_READ_CARD_ECC;
		}
		if (data[0] & 0x02) {
			sd_debug_print("* SD -- Failure: CMD17. Card CC failure during read. Error token: ", data, 1);
			return SD_ERROR_READ_CARD_CC;
		}
		sd_debug_print("* SD -- Failure: CMD17. Unknown error with single block read. Error token: ", data, 1);
		return SD_ERROR_READ_UNKNOWN;
	}

	/* Read a block length of data */
	for (i = 0; i < sd_block_len; i++) {
		data[i] = sd_spi_receive();
	}
	
	/* Read in the CRC16 */
	crc16 = (sd_spi_receive() << 8);
	crc16 |= sd_spi_receive();
	
	sd_spi_delay_clocks();

	/* Verify the data block's CRC */
	if (sd_crc16_data(data, sd_block_len) != crc16) {
		sd_debug_print("* SD -- Failure: CMD17. CRC16 invalid on read data block.", 0, 0);
		return SD_ERROR_READ_SINGLE_CRC;
	}

	sd_debug_print("* SD -- Success: CMD17. Retrieved single data block.", 0, 0);
	return 0;
}

int sd_read_status(uint16_t *sd_status) {
	uint8_t response[2];

	/* Send the read status command */
	sd_spi_command(SD_CMD13, 0x00, SD_CMD13_RL, response);
	
	/* Copy the status into the 16-bit variable */
	*sd_status = response[1];
	*sd_status |= (response[0] << 8);

	sd_debug_print("* SD -- Success: CMD13. Retrieved status register. Status: ", response, SD_CMD13_RL);

	return 0;	
}

int sd_pre_erase(uint32_t num_blocks) {
	uint8_t response[1];

	/* Only a 22-bit number of blocks can be specified */
	num_blocks = num_blocks & 0x3FFFFF;
	/* Minimum number of pre-erased blocks is 1 (default) */
	if (num_blocks == 0)
		num_blocks = 1;

	/* Send the pre-erase command with the specified
	 * number of blocks to be erased before writing. CMD55 + ACMD23 */
	sd_spi_command(SD_CMD55, 0x00, SD_CMD55_RL, response);
	if (response[0] != 0x00) {
		sd_debug_print("* SD -- Failure: ACMD23. Failure with app command. Response: ", response, SD_CMD55_RL);
		return SD_ERROR_APP_CMD;
	}

	sd_spi_command(SD_ACMD23, num_blocks, SD_ACMD23_RL, response);
	if (response[0] != 0x00) {
		sd_debug_print("* SD -- Failure: ACMD23. Unknown error. Response: ", response, SD_ACMD23_RL);
		return SD_ERROR_PRE_ERASE;
	}

	sd_debug_print("* SD -- Success: ACMD23. Set number of blocks to be pre-erased.", 0, 0);

	return 0;
}

int sd_set_block_len(uint32_t block_len) {
	uint8_t response[5];

	/* SD high capacity blocks have a fixed 512 byte block length */
	if (sd_high_capacity) {
		sd_block_len = SD_HCS_BLOCK_LENGTH;
		sd_debug_print("* SD -- Success: CMD16. Skipped setting block length because card is high capacity.", 0, 0);
		return 0;
	}

	/* Send the set block length command with our desired block length
	 * as the argument. */	
	sd_spi_command(SD_CMD16, block_len, SD_CMD16_RL, response);
	if (response[0] != 0x00) {
		sd_debug_print("* SD -- Failure: CMD16. Error setting block length. Response: ", response, SD_CMD16_RL);
		return SD_ERROR_SET_BLOCKLEN;
	}
	
	sd_block_len = block_len;

	sd_debug_print("* SD -- Success: CMD16. Block length set.", 0, 0);
	return 0;
}

int sd_is_mmc(void) {
	return sd_mmc;
}

int sd_get_block_len(void) {
	return sd_block_len;
}

int sd_get_high_capacity(void) {
	return sd_high_capacity;
}

int sd_get_size(void) {
	int retVal, i;
	uint8_t csd_read_block_len, csd_c_size_mult;
	uint32_t csd_c_size;

	retVal = sd_read_csd();
	if (retVal < 0)
		return retVal;

	/* High capacity and standard capacity encode different information
	 * to describe the size of the card. */
	if (sd_high_capacity) {
		/* Extract the 22-bit C_SIZE field */
		csd_c_size = (sd_csd[7] & 0x3F) << 16;
		csd_c_size |= (sd_csd[8] << 8);
		csd_c_size |= sd_csd[9];
		
		/* Now compute the size */

		/* Capacity = (C_SIZE+1)*512Kbyte */
		csd_c_size += 1;
		csd_c_size *= 524288;
	} else {
		/* Extract the READ_BLOCK_LEN, C_SIZE_MULT fields */
		csd_read_block_len = sd_csd[5] & 0x0F;
		csd_c_size_mult = (sd_csd[9] & 0x3) << 1;
		csd_c_size_mult |= ((sd_csd[10] & 0x80) >> 7);
	
		/* Extract the C_SIZE field */
		/* Upper byte of the C_SIZE field */
		csd_c_size = (((sd_csd[6] & 0x3) << 2) | ((sd_csd[7] & 0xC0) >> 6));
		csd_c_size <<= 8;
		/* Lower byte of the C_SIZE field */
		csd_c_size |= (((sd_csd[7] & 0x3F) << 2) | ((sd_csd[8] & 0xC0) >> 6));

		/* Now compute the size */
		/* Capacity = 
		 * 	(C_SIZE+1)*(2^(C_SIZE_MULT+2))*(2^(READ_BLOCK_LEN)) */
		csd_c_size += 1;
		
		/* *2^(csd_c_size_mult + 2) */
		csd_c_size_mult += 2;
		for (i = 0; i < csd_c_size_mult; i++)
			csd_c_size <<= 1;

		/* *2^(csd_read_block_len) */
		for (i = 0; i < csd_read_block_len; i++)
			csd_c_size <<= 1;
	}
	
	return csd_c_size;
}

int sd_init(void) {
	int sd_legacy, timeout, retVal;
	uint32_t argument;
	uint8_t response[5];

	timeout = 0;
	sd_high_capacity = 0;
	sd_block_len = 0;

	/* Initialize SPI bus */
	sd_spi_init();

	/* Send the idle command to put the SD card in idle mode */
	sd_spi_command(SD_CMD0, 0x00, SD_CMD0_RL, response);
	if (response[0] != 0x01) {
		sd_debug_print("* SD -- Failure: CMD0. Card did not enter idle mode. Response: ", response, SD_CMD0_RL);
		return SD_ERROR_IDLE;
	}
	sd_debug_print("* SD -- Success: CMD0. Card is in idle mode.", 0, 0);

	/* Send CMD8 to determine if we should follow 1.X or 2.X SD card initialization */

	/* The lower 0-7 bits are the check pattern, bits 8-11 is the supply voltage information,
	 * higher 12-31 bits are reserved. Supply voltage: 0001 for 2.7-3.6V. */
	argument = (1<<8) | SD_CHECK_PATTERN; 
	sd_spi_command(SD_CMD8, argument, SD_CMD8_RL, response);
	
	/* Check if the illegal command bit is set in the R1 portion of the response,
	 * if it was, remember that this is a legacy SD card. */
	if ((response[0] & (1<<2)) != 0x00) {
		sd_debug_print("* SD -- Failure: CMD8. Card is a 1.X card.", 0, 0);
		sd_legacy = 1;
	} else {
		sd_debug_print("* SD -- Success: CMD8. Card is a 2.X card.", 0, 0);
		sd_legacy = 0;
	}
	
	/* Check the CMD8 response if this is a 2.X SD card */
	if (sd_legacy == 0) {
		/* Check that the check pattern was correctly echo'd back */
		if (response[4] != SD_CHECK_PATTERN) {
			sd_debug_print("* SD -- Failure: CMD8. Pattern check invalid. Response: ", response, SD_CMD8_RL);
			return SD_ERROR_CHECK_PATTERN;
		}

		/* Check that the voltage range matches as well (correctly echo'd back) */
		if ((response[3] & 0x0F) != 0x01) {
			sd_debug_print("* SD -- Failure: CMD8. Unsupported voltage range. Response: ", response, SD_CMD8_RL);
			return SD_ERROR_VOLTAGE;
		}
	}
	
	/* Check the voltage range of the card with CMD58 */
	sd_spi_command(SD_CMD58, 0x00, SD_CMD58_RL, response);
	
	/* Check if the command returned illegal */
	if ((response[0] & (1<<2)) == (1<<2)) {
		sd_debug_print("* SD -- Failure: CMD58. Illegal command, not an SD card. Response: ", response, SD_CMD58_RL);
		sd_mmc = 1;
	} else {
		sd_mmc = 0;
	}
	
	/* Voltage range 3.2-3.3 is bit 20 of the packet, or bit 4 of the
	 * 3rd response byte. */
	if (!sd_mmc && ((response[2] & (1<<4)) != (1<<4))) {
		sd_debug_print("* SD -- Failure: CMD58. Unsupported voltage range. Response: ", response, SD_CMD58_RL);
		return SD_ERROR_VOLTAGE;
	}
	sd_debug_print("* SD -- Success: CMD58. Card supports voltage range.", 0, 0);

	if (!sd_legacy && SD_ENABLE_HCS)
		sd_debug_print("* SD -- Attempting to initialize with high capacity support...", 0, 0);

	/* Attempt ACMD41 initialization if this is not an MMC card */
	if (!sd_mmc) {
		for (timeout = 0; timeout < SD_INIT_TIMEOUT; timeout++) {
			sd_spi_command(SD_CMD55, 0x00, SD_CMD55_RL, response);
			//sd_spi_delay_clocks();
			if (response[0] == 0x01) {
				/* If high capacity support is enabled, and this is not a legacy SD card,
				 * turn on the 30th bit of the argument to initialize with high capacity support */
				if (!sd_legacy && SD_ENABLE_HCS)
					sd_spi_command(SD_ACMD41, (1<<30), SD_ACMD41_RL, response);
				else
					sd_spi_command(SD_ACMD41, 0x00, SD_ACMD41_RL, response);

				if ((response[0] & (1<<0)) == 0x00)
					break;
				//sd_spi_delay_clocks();
			}
		}
	
		/* Check if the command returned illegal */
		if ((response[0] & (1<<2)) == (1<<2)) {
			sd_debug_print("* SD -- Failure: ACMD41. Illegal command, not an SD card. Response: ", response, SD_CMD58_RL);
			return SD_ERROR_NOT_SD;
		}
	}

	/* Check if ACMD41 timed out */
	if (timeout == SD_INIT_TIMEOUT) {
		sd_debug_print("* SD -- Failure: ACMD41. Card initialization timed out. Response: ", response, SD_ACMD41_RL);
		sd_debug_print("* SD -- Attempting to initialize with CMD1...", 0, 0);
	}

	if (timeout == SD_INIT_TIMEOUT || sd_mmc) {
		/* Attempt CMD1 instead */
		for (timeout = 0; timeout < SD_INIT_TIMEOUT; timeout++) {
			/* If high capacity support is enabled, turn on the 30th bit of the argument */
			if (!sd_legacy && SD_ENABLE_HCS)
				sd_spi_command(SD_CMD1, (1<<30), SD_ACMD41_RL, response);
			else
				sd_spi_command(SD_CMD1, 0x00, SD_ACMD41_RL, response);

			if ((response[0] & (1<<0)) == 0x00)
				break;
			//sd_spi_delay_clocks();
		}
		/* Check if CMD1 timed out */
		if (timeout == SD_INIT_TIMEOUT) {
			sd_debug_print("* SD -- Failure: CMD1. Card initialization timed out. Response: ", response, SD_CMD1_RL);
			return SD_ERROR_INIT_TIMEOUT;
		}
	}

	sd_debug_print("* SD -- Success: ACMD41. Successfully initialized SD card.", 0, 0);

	if (!sd_legacy) {	
		/* Check the voltage range of the card with CMD58 */
		sd_spi_command(SD_CMD58, 0x00, SD_CMD58_RL, response);
		/* Check if the card has powered up and its capacity */
		if ((response[3] & (1<<7)) != 0x00) {
			/* Check cards' capacity */
			if ((response[3] & (1<<6)) != 0x00) {
				sd_debug_print("* SD -- Success: CMD58. Card is high capacity.", 0, 0);
				sd_high_capacity = 1;
			} else {
				sd_debug_print("* SD -- Success: CMD58. Card is standard capacity.", 0, 0);
				sd_high_capacity = 0;
			}
		}
	} else {
		/* Legacy SD cards can't be high capacity */
		sd_debug_print("* SD -- Legacy SD card. Card is standard capacity.", 0, 0);
		sd_high_capacity = 0;
	}

	/* Set our desired block length */
	retVal = sd_set_block_len(SD_BLOCK_LENGTH);
	if (retVal < 0)
		return retVal;

	/* Set the SPI Clock speed to 4MHz for data transfer
	 * PCLK = 60MHz, SPI Rate = 4MHz = 60/15, S0SPCCR = 15 */
	S0SPCCR = 15;

	return 0;
}

