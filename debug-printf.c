/* Debug Print Functions 
 *
 * Vanya A. Sergeev - <vsergeev@gmail.com>
 *
 */

#include "debug.h"
#include "uart.h"

#ifdef DEBUG

#define DEBUG_puts		UART0_puts
#define DEBUG_putc		UART0_putc

int debug_is_num(char c) {
	if (c >= '0' && c <= '9')
		return 1;
	return 0;
}

int debug_printf(char *fmt, ...) {
	va_list ap;

	/* String, unsigned int, and signed int arguments from va_arg */
	char *str;
	int sint;
	unsigned int uint;
	int sign;

	/* Used to convert to different bases */
	int i;
	char c;
	char numstr[34];

	/* Format char flag, zero padding flag, number of padding digits */
	int formatChar, zeroPadding, numPadding;
	/* Radix value for conversion of numbers to different bases */
	int radix;

	formatChar = zeroPadding = numPadding = 0;

	va_start(ap, fmt);

	for (; *fmt != 0; fmt++) {
		/* If we encounter a format definition */
		if (*fmt == '%') {
			/* Flip on the format char flag, reset the padding 
			 * digits, reset the radix, reset the sign. */
			formatChar = 1;
			zeroPadding = numPadding = radix = sign = 0;
			continue;
		}
		/* If we are in format character mode */
		if (formatChar) {
			/* If we detect a 0, we want zero padding */
			if (*fmt == '0') {
				zeroPadding = 1;
				continue;
			}

			/* If we previously detected a 0 for zero padding,
			 * and this next number is a digit, include it in
			 * our number of padding digits.  */
			if (zeroPadding && debug_is_num(*fmt)) {
				numPadding *= 10;
				numPadding += (*fmt - '0');
				continue;
			}

			/* Otherwise figure out what we need to format and
			 * print. */

			/* A string */
			if (*fmt == 's') {
				str = va_arg(ap, char *);
				for (; *str != '\0'; str++)
					DEBUG_putc(*str);

			/* An integer */
			} else if (*fmt == 'd') {
				sint = va_arg(ap, int);
				if (sint < 0) {
					uint = 0 - sint;
					sign = 1;
				} else {
					uint = sint;
				}
				radix = 10;

			/* A character */
			} else if (*fmt == 'c') {
				c = va_arg(ap, char);
				DEBUG_putc(c);

			/* An unsigned integer in base 10 */
			} else if (*fmt == 'u') {
				uint = va_arg(ap, unsigned int);
				radix = 10;

			/* An unsigned integer in base 16 */
			} else if (*fmt == 'x' || *fmt == 'X') {
				uint = va_arg(ap, unsigned int);
				radix = 16;

			/* An unsigned integer in base 2 */
			} else if (*fmt == 'b') {
				uint = va_arg(ap, unsigned int);
				radix = 2;
			}

			/* If we have to convert a number to
			 * a character representation in a particular base */
			if (radix != 0) {
				/* Null terminate our numeric string */
				numstr[sizeof(numstr)-1] = '\0';

				/* Keep on dividing through the number and
				 * collecting the modulus */
				i = sizeof(numstr)-1;
				do {
					/* Get the character representation of
					 * the current least significant
					 * digit. */
					c = (uint % radix) + '0';
					if (c > '9') {
						/* If we want lower case hex
						 * characters, shift to the
						 * lower case letters a-f */
						if (*fmt == 'x')
							c += 39;
						/* Otherwise, upper case */
						else
							c += 7;
					}
					/* Append this character backwards to
					 * our numeric string. */
					numstr[--i] = c;
					/* Divide through by the radix */
					uint /= radix;
				} while (uint != 0);

				/* Append the sign backwards */
				if (i > 0 && sign)
					numstr[--i] = '-';

				/* Add any any zero padding digits */
				/* Strip off any digits we've already used */
				numPadding -= (sizeof(numstr)-1-i);
				/* Pad the rest */
				for (; numPadding > 0 && i > 0; numPadding--)
					numstr[--i] = '0';

				/* Print the numeric string */
				DEBUG_puts(numstr+i);
			}
			
			/* Clear the format flag */	
			formatChar = 0;
		} else {
			/* Otherwise, just print a normal character */
			DEBUG_putc(*fmt);
		}
	}

	va_end(ap);
	return 0;
}

#else

int debug_printf(char *fmt, ...) {
	return 0;
}

#endif
