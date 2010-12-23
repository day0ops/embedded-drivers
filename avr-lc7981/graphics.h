/* 
 * Basic Raster Graphics interface to the lcd_graphics_* LCD graphics driver 
 * functions.
 * Vanya A. Sergeev - <vsergeev@gmail.com>
 *
 * font5x7.h header file comes from Procyon AVRlib.
 */

#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include "font5x7.h"

#define FONT_WIDTH	5
#define FONT_HEIGHT	7

void g_draw_horizontal_line(unsigned short x, unsigned short y, unsigned short length);
void g_draw_vertical_line(unsigned short x, unsigned short y, unsigned short length);
void g_draw_rectangle(unsigned short x, unsigned short y, unsigned short width, unsigned short height);
void g_draw_char(unsigned short x, unsigned short y, char character);
void g_draw_string(unsigned short x, unsigned short y, const char *str);

#endif
