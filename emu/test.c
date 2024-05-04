#include <stdio.h>
#include <stdlib.h>

#define LITTLE_ENDIAN
#include "emu.c"


int
interupt_0x10_putchar(Computer *c) {
    printf("putchar '%c'\n", c->ax.part.al);
    return 0;
}

int
interupt_0x20_exit(Computer *c) {
    puts("exit");
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
    c.interupt[0x10] = interupt_0x10_putchar;
    c.interupt[0x20] = interupt_0x20_exit;

    assert(argc == 2);

    filename = *(argv+1);
    /* printf("filename: %s\n", filename); */

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
    /* hexdump(c.memory, 0x0100, 50); */

    computer_dump(&c);

    for (i = 0; i < 100; i += 1) {
        if (computer_step(&c, 1)) {
            break;
        }
        computer_dump(&c);
    }

    /*
    puts("stack");
    hexdump(c.memory, 0xfee0, 0x20);
    */

	return 0;
}

