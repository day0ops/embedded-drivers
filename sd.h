/* SD/SPI driver for LPC2109
 * 
 * Vanya A. Sergeev - <vsergeev@gmail.com> - copyright 2010
 * please inform author of possible use, licensing is still being decided
 *
 */

/* Following features of the SD spec have been omitted:
	* Set/Clear/Inquire write protection blocks
	* Lock/Unlock via password
	* Program CSD
	* Switch card function (CMD6)
	* Command CRC on/off (CRC is always computed and encoded)
	* All application commands except for initialization (ACMD41)
 */

#include "lpc21xx.h"
#include "stdint.h"

/* Arbitrary SD check pattern data */
#define SD_CHECK_PATTERN	0x55
/* Slightly arbitrary initialization timeout count */
#define SD_INIT_TIMEOUT		900
/* Enable or disable high capacity support */
#define SD_ENABLE_HCS		1
/* Desired block length */
#define SD_BLOCK_LENGTH		512

/* Debugging options */
//#define SD_DEBUG

/* SD Chip Select pin defnitions */
#define SD_CS_IODIR		FIO0DIR
#define SD_CS_IOSET		FIO0SET
#define SD_CS_IOCLR		FIO0CLR
#define SD_CS_PIN		(1<<7)

/* SD Chip Select macros */
#define sd_spi_select()		(SD_CS_IOCLR = SD_CS_PIN)
#define sd_spi_deselect()	(SD_CS_IOSET = SD_CS_PIN) 

/* Constants associated with the SD SPI protocol */
#define SD_SPI_CMD_READ_ATTEMPTS		10
#define SD_SPI_DATA_READ_ATTEMPTS		100
#define SD_SPI_DATA_BLOCK_START			0xFE
#define SD_SPI_MULTIPLE_DATA_BLOCK_START	0xFC
#define SD_SPI_MULTIPLE_DATA_BLOCK_END		0xFD
	/* 0xE0 instead of just 0xF0 to support MMC */
#define SD_SPI_DATA_ERROR_TOKEN_MASK		0xE0
#define SD_SPI_BUSY				0x00
#define SD_CSD_LENGTH				16
#define SD_CID_LENGTH				16
#define SD_HCS_BLOCK_LENGTH			512

/* SD SPI Commands */

/* SD Data Response Token statuses: xxx0sss1 */
#define SD_SPI_WRITE_ACCEPTED		0x02
#define SD_SPI_WRITE_ERROR_CRC		0x05
#define SD_SPI_WRITE_ERROR_WRITE	0x06

/* Response byte lengths */
#define SD_R1	1
#define SD_R1b	1
#define	SD_R2	2
#define SD_R3	5
#define SD_R7	5

/* Reset SD card */
#define SD_CMD0		0
#define SD_CMD0_RL	SD_R1
/* Send host capacity support information, start SD
 * card initialization. */
#define SD_CMD1		1
#define SD_CMD1_RL	SD_R1/* Send host supply voltage information, and if SD card
 * can operate in this voltage range. */
#define SD_CMD8		8
#define SD_CMD8_RL	SD_R7
/* Send CSD */
#define SD_CMD9		9
#define SD_CMD9_RL	SD_R1
/* Send CID */
#define SD_CMD10	10
#define SD_CMD10_RL	SD_R1
/* Stop multiple block transmission */
#define SD_CMD12	12
#define SD_CMD12_RL	SD_R1
/* Read Status Register */
#define SD_CMD13	13
#define SD_CMD13_RL	SD_R2
/* Set block length */
#define SD_CMD16	16
#define SD_CMD16_RL	SD_R1
/* Read single block */
#define SD_CMD17	17
#define SD_CMD17_RL	SD_R1
/* Read multiple blocks */
#define SD_CMD18	18
#define SD_CMD18_RL	SD_R1
/* Write single block */
#define SD_CMD24	24
#define SD_CMD24_RL	SD_R1
/* Write multiple blocks */
#define SD_CMD25	25
#define SD_CMD25_RL	SD_R1
/* SD Erase address start */
#define SD_CMD32	32
#define SD_CMD32_RL	SD_R1
/* SD Erase address end */
#define SD_CMD33	33
#define SD_CMD33_RL	SD_R1
/* MMC Erase address start */
#define SD_CMD35	35
#define SD_CMD35_RL	SD_R1
/* MMC Erase address end */
#define SD_CMD36	36
#define SD_CMD36_RL	SD_R1
/* Erase */
#define SD_CMD38	38
#define SD_CMD38_RL	SD_R1b
/* Specify that the next command is an application
 * command. */
#define SD_CMD55	55
#define SD_CMD55_RL	SD_R1
/* Read OCR register of the SD card. */
#define SD_CMD58	58
#define SD_CMD58_RL	SD_R3
/* Turn on/off the CRC option. */
#define SD_CMD59	59
#define SD_CMD59_RL	SD_R1
/* Set pre-erase write blocks. */
#define SD_ACMD23	23
#define SD_ACMD23_RL	SD_R1
/* Send host capacity support information, start SD
 * card initialization. */
#define SD_ACMD41	41
#define SD_ACMD41_RL	SD_R1

/* Possible SD card initialization and command errors */
enum SD_ERRORS {
	SD_ERROR_IDLE		 = -1,
	SD_ERROR_VOLTAGE	 = -2,
	SD_ERROR_CHECK_PATTERN	 = -3,
	SD_ERROR_INIT_TIMEOUT	 = -4,
	SD_ERROR_NOT_SD		 = -5,

	SD_ERROR_SET_BLOCKLEN	 = -6,
	SD_ERROR_GET_CSD	 = -7,
	SD_ERROR_GET_CSD_CRC	 = -8,
	SD_ERROR_GET_CID	 = -9,
	SD_ERROR_GET_CID_CRC	 = -10,

	SD_ERROR_START_ADDR_MISALIGNED	 = -11,
	SD_ERROR_START_ADDR_OUTBOUNDS	 = -12,
	SD_ERROR_START_ADDR_UNKNOWN	 = -13,
	SD_ERROR_END_ADDR_MISALIGNED	 = -14,
	SD_ERROR_END_ADDR_OUTBOUNDS	 = -15,
	SD_ERROR_END_ADDR_UNKNOWN	 = -16,
	SD_ERROR_ERASE			 = -17,

	SD_ERROR_READ_DATALEN_MULTIPLE	 = -18,
	SD_ERROR_READ_ADDR_MISALIGNED	 = -19,
	SD_ERROR_READ_ADDR_OUTBOUNDS	 = -20,
	SD_ERROR_READ_UNKNOWN		 = -21,
	SD_ERROR_READ_OUT_OF_RANGE	 = -22,
	SD_ERROR_READ_CARD_ECC		 = -23,
	SD_ERROR_READ_CARD_CC		 = -24,
	SD_ERROR_READ_SINGLE_CRC	 = -25,
	SD_ERROR_READ_MULTIPLE_CRC	 = -26,

	SD_ERROR_STOP_MULTIPLE_BLOCKS	 = -27,

	SD_ERROR_WRITE_ADDR_MISALIGNED	 = -28,
	SD_ERROR_WRITE_ADDR_OUTBOUNDS	 = -29,
	SD_ERROR_WRITE_DATALEN_MULTIPLE	 = -30,
	SD_ERROR_WRITE_BLOCK		 = -31,
	SD_ERROR_WRITE_BLOCK_CRC	 = -32,
	SD_ERROR_WRITE_UNKNOWN		 = -33,

	SD_ERROR_APP_CMD		 = -34,
	SD_ERROR_PRE_ERASE		 = -35,
};

/* SD Status Register error bits */
enum SD_Status_Errors {
	SD_STATUS_ERROR_CARD_LOCKED	= (1<<0),
	SD_STATUS_ERROR_WP_ERASE	= (1<<1),
	SD_STATUS_ERROR_UNKNOWN_ERROR	= (1<<2),
	SD_STATUS_ERROR_CARD_CC		= (1<<3),
	SD_STATUS_ERROR_CARD_ECC	= (1<<4),
	SD_STATUS_ERROR_WP_WRITE	= (1<<5),
	SD_STATUS_ERROR_ERASE_SEL	= (1<<6),
	SD_STATUS_ERROR_OUT_OF_RANGE	= (1<<7),
	SD_STATUS_ERROR_IDLE		= (1<<8),
	SD_STATUS_ERROR_ERASE_RESET	= (1<<9),
	SD_STATUS_ERROR_ILLEGAL_CMD	= (1<<10),
	SD_STATUS_ERROR_COM_CRC		= (1<<11),
	SD_STATUS_ERROR_ERASE_SEQ	= (1<<12),
	SD_STATUS_ERROR_ADDR_ERROR	= (1<<13),
	SD_STATUS_ERROR_PARAM_ERROR	= (1<<14),
};

void sd_cd_wp_init(void);
uint8_t sd_card_detect(void);
uint8_t sd_write_protect(void);

void sd_spi_init(void);
void sd_spi_send(uint8_t data);
uint8_t sd_spi_receive(void);
void sd_spi_delay_clocks(void);
uint8_t sd_crc7_bits(uint8_t data, uint8_t seed);
uint8_t sd_crc7_packet(const uint8_t *data, int dataLen);
uint16_t sd_crc16_bits(uint8_t data, uint16_t seed);
uint16_t sd_crc16_data(const uint8_t *data, int dataLen);
void sd_spi_command(uint8_t command, uint32_t argument, int responseLength, uint8_t *response);

void sd_debug_print_boolean(uint8_t data);
void sd_debug_print_csd(void);
void sd_debug_print_cid(void);
void sd_debug_print_data_block(uint8_t *data);
void sd_debug_print(char *message, uint8_t *data, int dataLen);

int sd_read_csd(void);
int sd_read_cid(void);
int sd_write_block(uint32_t address, const uint8_t *data);
int sd_write_blocks(uint32_t address, const uint8_t *data, int dataLen);
int sd_read_block(uint32_t address, uint8_t *data);
int sd_read_blocks(uint32_t address, uint8_t *data, int dataLen);
int sd_pre_erase(uint32_t num_blocks);
int sd_read_status(uint16_t *sd_status);
int sd_is_mmc(void);
int sd_get_block_len(void);
int sd_get_high_capacity(void);
int sd_get_size(void);
int sd_set_block_len(uint32_t block_len);
int sd_init(void);
int sd_erase_blocks(uint32_t address_start, uint32_t address_end);

