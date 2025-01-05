#include <stdio.h>
#include <stdlib.h>

//#define LITTLE_ENDIAN
#include "emu.c"

// Initialize the screen
void
screen_init(Screen *screen) {
    memset(screen->lines, ' ', sizeof(screen->lines)); // Fill with spaces
    screen->current_line = 0;
    screen->x = 0;
    screen->y = 0;
}

// Print the entire screen
void
screen_print(const Screen *screen) {
    int i = 0;
#define HLINE_WIDTH (SCREEN_WIDTH+2)
    char s[HLINE_WIDTH] = "";
    memset(s, '-', HLINE_WIDTH); 
    s[0] = '+';
    s[HLINE_WIDTH-1] = '+';
    s[HLINE_WIDTH] = '\0';

	puts(s);
    for (i = 0; i < SCREEN_HEIGHT; i++) {
        int line_index = (screen->current_line + i) % SCREEN_HEIGHT;
        printf("|%.*s|\n", SCREEN_WIDTH, screen->lines[line_index]);
    }
	puts(s);
}

// Put a character on the screen and handle the buffer
void
screen_putchar(Screen *screen, char c) {
    if (c == '\n') { // Handle newline
        screen->x = 0;
        screen->y = (screen->y + 1) % SCREEN_HEIGHT;

        if (screen->y == 0) { // Wrap around, move the current_line
            screen->current_line = (screen->current_line + 1) % SCREEN_HEIGHT;
        }
    } else { // Handle regular characters
        screen->lines[screen->y][screen->x] = c;
        screen->x++;

        if (screen->x >= SCREEN_WIDTH) { // Move to the next line if the line is full
            screen->x = 0;
            screen->y = (screen->y + 1) % SCREEN_HEIGHT;

            if (screen->y == 0) {
                screen->current_line = (screen->current_line + 1) % SCREEN_HEIGHT;
            }
        }
    }
}

// Emulated interrupt 0x10 function to put a character
int
interupt_0x10_putchar(Computer *c) {
    screen_putchar(&c->screen, c->ax.part.al);
    return 0;
}


int
interupt_0x20_exit(Computer *c) {
	puts("");
	puts("exit interupt");
	return 1;
}

int
main(int argc, char **argv) {
    FILE *file = NULL;
    char *filename = NULL;
    size_t file_size = 0;
    char *rom = NULL;
    int i = 0;

    Computer c;

    computer_init(&c);

    screen_init(&c.screen);

    c.interupt[0x10] = interupt_0x10_putchar;
    c.interupt[0x20] = interupt_0x20_exit;

    assert(argc == 2);

    filename = *(argv+1);
    printf("filename: %s\n", filename);

    file = fopen(filename, "rb");
    if (file == NULL) {
        assert(false);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    rom = malloc(file_size);
    if (rom == NULL) {
        assert(false);
        return 1;
    }

    fread(rom, 1, file_size, file);
    fclose(file);

    computer_memcpy(&c, 0x0100, rom, file_size);
    hexdump(c.memory, 0x0100, 50);

    puts("");
	puts(
            "ax   "
            "bx   "
            "cx   "
            "dx   "
            "si   "
            "di   "
            "bp   "
            "sp   "
            "ip   "
            "flags "
	    );
    // computer_dump(&c);

	computer_dump(&c);
    for (i = 0; i < 130; i += 1) {
        if (computer_step(&c, 1)) {
            break;
        }
        computer_dump(&c);
    }
    screen_print(&c.screen);

    // computer_dump(&c);

    /*
    puts("stack");
    hexdump(c.memory, 0xfee0, 0x20);
    */

	return 0;
}

