/******************************************************************************
 * LC7981/HD61830 Graphics LCD Driver
 *
 * Configured for the Samsung LJ41-00192B 160x80 monochrome graphics lcd.
 *
 * Written by Vanya A. Sergeev - <vsergeev@gmail.com>                         
 *
 * February 16, 2007                                                       
 *
***************************************************************************/

/* lc7981.h: Header file of the LC7981/HD61830 graphics lcd driver. Contains 
 * the hardware definitions and function prototypes for the graphics lcd 
 * driver. */ 

/* AVR clock speed */
#define F_CPU	8000000

#include <avr/io.h>
#include <util/delay.h>

/* Hardware setup */
#define LCD_DATA_DDR	DDRA
#define LCD_DATA_PORT	PORTA
#define LCD_DATA_PIN	PINA

#define LCD_CTRL_DDR	DDRB
#define LCD_CTRL_PORT	PORTB

#define LCD_CTRL_RS	1
#define LCD_CTRL_RW	2
#define LCD_CTRL_E	4

#define LCD_WIDTH	160
#define LCD_HEIGHT	80

/* Convenient macros to toggle RS, RW, and E control pins */
#define lcd_rs_high() (LCD_CTRL_PORT |= (1<<LCD_CTRL_RS))
#define lcd_rs_low() (LCD_CTRL_PORT &= ~(1<<LCD_CTRL_RS))

#define lcd_rw_high() (LCD_CTRL_PORT |= (1<<LCD_CTRL_RW))
#define lcd_rw_low() (LCD_CTRL_PORT &= ~(1<<LCD_CTRL_RW))

#define lcd_enable_high() (LCD_CTRL_PORT |= (1<<LCD_CTRL_E))
#define lcd_enable_low() (LCD_CTRL_PORT &= ~(1<<LCD_CTRL_E))

/* All possible instructions for the LCD Instruction Register */
#define	LCD_CMD_MODE			0x00
#define LCD_CMD_CHAR_PITCH		0x01
#define LCD_CMD_NUM_CHARS		0x02
#define LCD_CMD_TIME_DIVISION		0x03
#define LCD_CMD_CURSOR_POS		0x04

#define LCD_CMD_DISPLAY_START_LA	0x08
#define LCD_CMD_DISPLAY_START_HA	0x09
#define LCD_CMD_CURSOR_LA		0x0A
#define LCD_CMD_CURSOR_HA		0x0B

#define LCD_CMD_WRITE_DATA		0x0C
#define LCD_CMD_READ_DATA		0x0D

#define LCD_CMD_CLEAR_BIT		0x0E
#define LCD_CMD_SET_BIT			0x0F

/* Bits of the LCD Mode Control Register (DB5-DB0) */
#define LCD_MODE_ON_OFF		32
#define LCD_MODE_MASTER_SLAVE	16
#define LCD_MODE_BLINK		8
#define LCD_MODE_CURSOR		4
#define LCD_MODE_MODE		2
#define LCD_MODE_EXTERNAL	1

/* Possible settings of the character pitch register */
/* Horizontal character pitches of 6, 7, and 8 */
#define LCD_CHAR_PITCH_HP_6	0x05
#define LCD_CHAR_PITCH_HP_7	0x06
#define LCD_CHAR_PITCH_HP_8	0x07

/* To indicate the state of a pixel */
#define PIXEL_ON	0xFF
#define PIXEL_OFF	0x00

/* Low-level LCD interface functions */
void lcd_strobe_enable(void);
void lcd_write_command(unsigned char command, unsigned char data);
void lcd_wait_busy(void);

/* LCD Graphics interface */
void lcd_graphics_init(void);
void lcd_graphics_clear(void);
void lcd_graphics_move(unsigned short x, unsigned short y);
void lcd_graphics_draw_byte(unsigned char data);
void lcd_graphics_plot_byte(unsigned short x, unsigned short y, unsigned char data);
void lcd_graphics_plot_pixel(unsigned short x, unsigned short y, unsigned char state);

