#include "penguin.xpm"

int main(void) {
	int i, j;
	printf("void draw_penguin(void) {\n");
	for (i = 0; i < 80; i++) {
		for (j = 0; j < 80; j++) {
			if (penguin[i][j] == 'B')
				printf("lcd_graphics_plot_pixel(%d, %d, PIXEL_ON);\n", j+84, i);
		}
	}
	printf("}\n");
	return 0;
}
