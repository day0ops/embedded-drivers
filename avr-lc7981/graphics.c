/* 
 * Basic Raster Graphics interface to the lcd_graphics_* LCD graphics driver 
 * functions.
 * Vanya A. Sergeev - <vsergeev@gmail.com>
 *
 * font5x7.h header file comes from Procyon AVRlib.
 *
 */

#include "lc7981.h"
#include "graphics.h"

/**
 * Draws a horizontal line, left to right, at the specified coordinates and of
 * the specified length.
 * @param x The x coordinante of the line's origin.
 * @param y The y coordinante of the line's origin.
 * @param length The length of the line, in pixels.
 */
void g_draw_horizontal_line(unsigned short x, unsigned short y, unsigned short length) {
	unsigned short i;
	for (i = x; i <= x+length; i++)
		lcd_graphics_plot_pixel(i, y, PIXEL_ON);
}

/**
 * Draws a vertical line, top to bottom, at the specified coordinates and of 
 * the specified length.
 * @param x The x coordinante of the line's origin.
 * @param y The y coordinante of the line's origin.
 * @param length The length of the line, in pixels.
 */
void g_draw_vertical_line(unsigned short x, unsigned short y, unsigned short length) {
	unsigned short i;
	for (i = y; i < y+length; i++)
		lcd_graphics_plot_pixel(x, i, PIXEL_ON);
}

/**
 * Draws a rectangle. The coordinantes specify the top left corner of the 
 * rectangle.
 * @param x The x coordinante of the rectangle's origin.
 * @param y The y coordinante of the rectangle's origin.
 * @param width The width of the rectangle, in pixels.
 * @param height The height of the rectangle, in pixels.
 */ 
void g_draw_rectangle(unsigned short x, unsigned short y, unsigned short width, unsigned short height) {
	/* Adjust width and height because x and y coordinates start on 0,0 */
	width--;
	height--;
	g_draw_horizontal_line(x, y, width);
	g_draw_vertical_line(x, y, height);
	g_draw_vertical_line(x+width, y, height);
	g_draw_horizontal_line(x, y+height, width);
}

/**
 * Draws a 5x7 character to the screen. The coordinantes specify the top left 
 * corner of the character.
 * Characters outside the character set will not be drawn (function will return
 * immediately).
 * @param x The x coordinate of the character's origin.
 * @param y The y coordinate of the character's origin.
 * @param character The ASCII character to draw.
 */
void g_draw_char(unsigned short x, unsigned short y, char character) {
	unsigned char fontIndex, i, j;

	/* The 5x7 character set starts at the '!' character (ASCII index 
 	 * number 33) so we subtract 32 from the ASCII character to find the 
 	 * index in the 5x7 font table. */	
	fontIndex = character-32;
	/* If the index is out of bounds, bail out */
	if (fontIndex > 94)
		return;
	
	for (i = 0; i < FONT_WIDTH; i++) {
		for (j = 0; j < FONT_HEIGHT; j++) {
			/* Check if the bit/pixel is set, paint accoringly to 
 			 * the screen */
			if (Font5x7[FONT_WIDTH*fontIndex+i] & (1<<j))
				lcd_graphics_plot_pixel(x, y+j, PIXEL_ON);
			else	
				lcd_graphics_plot_pixel(x, y+j, PIXEL_OFF);
		}
		/* Move the LCD cursor through the font width as well */
		x++;
	}
}

/**
 * Draws a string of characters to the screen. The characters will be drawn in i
 * 5x7 font.
 * The coordinantes specify the top left corner of the first character in the 
 * string.
 * Characters will be wrapped. The newline character is supported.
 * Characters outside the 5x7 character set will not be drawn.
 * @param x The x coordinate of the string's first character's origin.
 * @param y The y coordinate of the string's first character's origin.
 * @param str The null-terminated ASCII string of characters.
 */
void g_draw_string(unsigned short x, unsigned short y, const char *str) {
	unsigned short origin_X;

	/* Preserve the origin X, in case of a new line */
	origin_X = x;

	/* Continue through the string until we encounter a null character */
	while (*str != '\0') {
		/* If the character is a newline, then prepare our x and y
		 * coordinates for the next character on the new line. */
		if (*str == '\n') {
			/* Reset x to its origin */
			x = origin_X;
			/* Move y one character down */
			y += FONT_HEIGHT+1;
		
			str++;
			continue;
		}
		g_draw_char(x, y, *str++);
		
		/* Add a 1-pixel spacing between the characters */
		x += FONT_WIDTH+1;

		/* In case we are putting this character out of bounds,
		 * move the character to the next line on the display */
		if ((x+FONT_WIDTH) > LCD_WIDTH) {
			/* Reset x to its origin */
			x = origin_X;
			/* Move y one character down */
			y += FONT_HEIGHT+1;
		}
	}
}
