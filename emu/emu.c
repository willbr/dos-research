#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef _MSC_VER
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

DLLEXPORT void myFunction() {
    // function implementation
}

#define false 0
#define true (!false)

#define mask_flag_z 0x80
#define mask_flag_h 0x40
#define mask_flag_n 0x20
#define mask_flag_c 0x10

#define computer_set_flag(c, flag) \
        (c->flags |= flag)

#define computer_clear_flag(c, flag) \
        (c->flags &= ~(flag))


#define CPU_STATE_FIELDS \
    X("%04x", c->ax.u16) \
    X("%04x", c->bx.u16) \
    X("%04x", c->cx.u16) \
    X("%04x", c->dx.u16) \
    X("%04x", c->si) \
    X("%04x", c->di) \
    X("%04x", c->bp) \
    X("%04x", c->sp) \
    X("%04x", c->ip) \
    X("%04x", c->flags) \
    X("| %-10s", opcode_bytes) \
    X("%-10s", assembly)

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 24


typedef unsigned char byte;

typedef unsigned char  u8;
typedef   signed char  s8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long long u64;


#if defined(LITTLE_ENDIAN)
struct RegisterAX { u8 al; u8 ah; };
struct RegisterBX { u8 bl; u8 bh; };
struct RegisterCX { u8 cl; u8 ch; };
struct RegisterDX { u8 dl; u8 dh; };
#elif defined(BIG_ENDIAN)
struct RegisterAX { u8 ah; u8 al; };
struct RegisterBX { u8 bh; u8 bl; };
struct RegisterCX { u8 ch; u8 cl; };
struct RegisterDX { u8 dh; u8 dl; };
#else
#error "Please define LITTLE_ENDIAN or BIG_ENDIAN"
#endif

struct Computer;
typedef int (*interupt_callback)(struct Computer*);

typedef struct {
    char lines[SCREEN_HEIGHT][SCREEN_WIDTH]; // Ring buffer for the screen
    int current_line;                        // Index of the top-most line in the ring buffer
    int x, y;                                // Cursor position
} Screen;


typedef struct {
        u16 channel0_value;
} PIT;

typedef struct Computer {
    union { u16 u16; struct RegisterAX part; } ax; /* accumulator */
    union { u16 u16; struct RegisterBX part; } bx; /* address     */
    union { u16 u16; struct RegisterCX part; } cx; /* coutner     */
    union { u16 u16; struct RegisterDX part; } dx; /* 32bit extention of ax */
    u16 si; /* source address */
    u16 di; /* target address */
    u16 bp; /* base pointer   */
    u16 sp; /* stack pointer  */
    u16 ip; /* instruction pointer  */
    u16 flags; /* stack pointer  */
    u16 es; /* source address */
    u16 cs; /* source address */
    u16 ss; /* source address */
    u16 ds; /* source address */
    u8 memory[0x10000];
    interupt_callback interupt[0xff];
    Screen screen;
    PIT pit;
} Computer;


typedef struct OpcodeOperand {
    int name;
    //int  bytes;
    //int  immediate;
    //int  increment;
    //int  decrement;
} OpcodeOperand;


typedef struct OpcodeDef {
    char mnemonic[20];
    int num_bytes;
    //int cycle_steps;
    //int cycles[2];
    int num_operands;
    OpcodeOperand operands[3];
    //int immediate;
    //char flag_z;
    //char flag_n;
    //char flag_h;
    //char flag_c;
} OpcodeDef;


#include "opcodes.c"

void hexdump(u8 *src, int offset, int size);

DLLEXPORT
u32 
computer_init(Computer *c) {
    int i = 0;

    c->ax.u16 = 0;
    c->bx.u16 = 0;
    c->cx.u16 = 0;
    c->dx.u16 = 0;

    c->si = 0;
    c->di = 0;
    c->bp = 0;
    c->sp = 0xff00;
    c->ip = 0x0100;
    c->flags = 0;

    for (i = 0; i < 256; i += 1) {
        c->interupt[i] = NULL;
    }

    memset(c->memory, 0, sizeof(c->memory));

    return 0;
}

u16
computer_read_u16le(Computer *gb, u16 offset) {
    u8 low  = 0;
    u8 high = 0;
    u16 n16   = 0;

    low  = gb->memory[offset];
    high = gb->memory[offset + 1];

    n16 = (high << 8) + low;

    return n16;
}





DLLEXPORT
void * 
computer_memcpy(
    Computer *c,
    size_t offset,
    const void *src,
    size_t src_size
    ) {

    void *rval  = NULL;
    void *dst   = NULL;
    void *end   = NULL;
    void *limit = NULL;

    limit = &c->memory[0] + sizeof(c->memory);

    dst = &c->memory[offset];
    end = dst + src_size;

    if (end >= limit) {
        printf("dst   = %p\n", dst);
        printf("end   = %p\n", end);
        printf("limit = %p\n", limit);
        puts("doesnt fit");
        return NULL;
    }

    rval = memcpy(dst, src, src_size);

    return rval;
}


void
computer_dis_operand(
                Computer *c,
                u16 offset,
                char *s,
                OpcodeOperand *op) {

    s8  e8  = 0;
    u8  n8  = 0;
    u16 n16 = 0;
    u16 a16 = 0;
    char c1, c2, c3, c4;
    
    //printf("operand %c %c\n", (op->name & 0xff00) >> 8, op->name & 0xff);

    switch (op->name) {
    case 'e8':
        e8 = c->memory[offset+1];
        // sprintf(s, "%d", e8);
        a16 = offset + 2 + e8;
        sprintf(s, "0x%02x", a16);
        break;

    case 'n8':
        n8 = c->memory[offset+1];
        sprintf(s, "0x%02x", n8);
        break;

    case 'n16':
        n16 = computer_read_u16le(c, offset+1);
        sprintf(s, "0x%04x", n16);
        break;

    default:
        c1 = (op->name >> 24) & 0xff;
        c2 = (op->name >> 16) & 0xff;
        c3 = (op->name >> 8) & 0xff;
        c4 = op->name & 0xff;

        if (op->name <= 0xff) {
            sprintf(s, "%c", c4);
        } else if (op->name <= 0xffff) {
            sprintf(s, "%c%c", c3, c4);
        } else if (op->name <= 0xffffff) {
            sprintf(s, "%c%c%c", c2, c3, c4);
        } else if (op->name <= 0xffffffff) {
            sprintf(s, "%c%c%c%c", c1, c2, c3, c4);
        } else {
            printf("default");
        }

        break;
    }
/*

        u16 a16 = 0;
        u8  n8  = 0;
        s8  e8  = 0;

        char prefix[2] = "";
        char suffix[2] = "";
        char mod[2] = "";

        if (op->immediate == false) {
                prefix[0] = '[';
                suffix[0] = ']';
        }

        if (op->increment) {
                mod[0] = '+';
        } else if (op->decrement) {
                mod[0] = '-';
        }

        if (strcmp(op->name, "a16") == 0) {
                a16 = computer_read_u16le(gb, offset+1);
                sprintf(s, "%s0x%04x%s", prefix, a16, suffix);
        } else if (strcmp(op->name, "n16") == 0) {
                n16 = computer_read_u16le(gb, offset+1);
                sprintf(s, "%s0x%04x%s", prefix, n16, suffix);
        } else if (strcmp(op->name, "n8") == 0) {
                n8 = computer_read_u8(gb, offset+1);
                sprintf(s, "%s0x%02x%s", prefix, n8, suffix);
        } else if (strcmp(op->name, "e8") == 0) {
                n8 = computer_read_u8(gb, offset+1);
                e8 = n8;
                a16 = gb->ip + 2 + e8;
                sprintf(s, "%s%04x%s", prefix, a16, suffix);
        } else {
                sprintf(s, "%s%s%s%s", prefix, op->name, mod, suffix);
        }
*/
}

void
computer_dis(
    Computer *c,
    u16 offset,
    char *opcode_bytes,
    char *assembly) {

    static char op1[10] = "";
    static char op2[10] = "";
    static char op3[10] = "";

    OpcodeDef *spec = NULL;
    u8 *ip = &c->memory[offset];

        spec = &opcodes[*ip];

    switch (*ip) {
    case 0x84: /* test */
        switch (*(ip + 1)) {
        case 0xc0:
            sprintf(opcode_bytes, "%02x %02x   ", *ip, *(ip + 1));
            sprintf(
                assembly,
                "%-3s al, al      ",
                spec->mnemonic
                );
            return;
        default:
            break;
        }
        break;

    default:
        break;
    }

    switch (spec->num_bytes) {
    case 0:
        sprintf(opcode_bytes, "%02x      ", *ip);
        break;
    case 1:
        sprintf(opcode_bytes, "%02x      ", *ip);
        break;
    case 2:
        sprintf(opcode_bytes, "%02x %02x   ", *ip, *(ip + 1));
        break;
    case 3:
        sprintf(opcode_bytes,
            "%02x %02x %02x",
            *ip,
            *(ip + 1),
            *(ip + 2));
        break;
    default:
        printf("num_bytes = %d\n", spec->num_bytes);
        assert(false);
    }

    switch (spec->num_operands) {
    case 0:
        sprintf(assembly, "%s         ", spec->mnemonic);
        break;
    case 1:
        computer_dis_operand(c, offset, op1, &spec->operands[0]);
        sprintf(
            assembly,
            "%-3s %s      ",
            spec->mnemonic,
            op1
            );
        break;
    case 2:
        computer_dis_operand(c, offset, op1, &spec->operands[0]);
        computer_dis_operand(c, offset, op2, &spec->operands[1]);
        sprintf(
            assembly,
            "%-3s %s, %s   ",
            spec->mnemonic,
            op1,
            op2
            );
        break;
    case 3:
        computer_dis_operand(c, offset, op1, &spec->operands[0]);
        computer_dis_operand(c, offset, op2, &spec->operands[1]);
        computer_dis_operand(c, offset, op3, &spec->operands[2]);
        sprintf(
            assembly,
            "%-3s %s, %s, %s",
            spec->mnemonic,
            op1,
            op2,
            op3
            );
        break;
    default:
        assert(false);
    }

}

DLLEXPORT
u32
computer_dump(Computer *c) {
    u16 file_offset = 0;
    static char opcode_bytes[9] = "op";
    static char assembly[20] = "asm";
    /*
    static char flags[5] = "----";
    u8 deref_hl = gb->memory[gb->hl.u16];

    flags[0] = gb->af.part.f & mask_flag_z ? 'z' : '-';
    flags[1] = gb->af.part.f & mask_flag_h ? 'h' : '-';
    flags[2] = gb->af.part.f & mask_flag_n ? 'n' : '-';
    flags[3] = gb->af.part.f & mask_flag_c ? 'c' : '-';
        */

    computer_dis(c, c->ip, opcode_bytes, assembly);

    printf(
#define X(format, value) format " "
    CPU_STATE_FIELDS
#undef X
    "\n"
    // Define X for generating the argument list
#define X(format, value) , value
    CPU_STATE_FIELDS
#undef X
    );

    return 0;
}


void
computer_push(Computer *c, u16 value) {
    c->sp -= 2;
    c->memory[c->sp] = value & 0xff;
    c->memory[c->sp + 1] = (value >> 8) & 0xff;
}


void
computer_pop(Computer *c, u16 *ptr_value) {
    u16 n16;
    n16 = c->memory[c->sp] + (c->memory[c->sp + 1] << 8);
    *ptr_value = n16;
    c->sp += 2;
}


u16
computer_read_port(Computer *c, u8 port) {
    if (port == 0x40) {
        return c->pit.channel0_value;
    }
    fprintf(stderr, "Unknown I/O port 0x%02x\n", port);
    return 0xFFFF; // Default error value
}

u16
decode_effective_address(Computer *c, u8 modrm) {
    // Decode the effective address based on the ModR/M byte
    // This function would handle addressing modes like [BX], [SI], [DI], etc.
    // Placeholder implementation:
    return 0x1000; // Example fixed address (replace with actual decoding logic)
}


DLLEXPORT
u32
computer_step(Computer *c, u32 steps) {
    u32 i          = 0;
    u8 opcode      = 0;
    u8 param       = 0;
    u16 n8         = 0;
    u16 n16        = 0;
    u16 a16        = 0;
    s8  e8         = 0;
    u8 port        = 0;

    for (i = 0; i < steps; i += 1) {
        opcode = c->memory[c->ip];
        /* printf("opcode %x\n", opcode); */
        switch (opcode) {
            case 0x40: /* inc ax */
                c->ax.u16 += 1;
                c->ip += 1;
                break;

            case 0x43: /* inc bx */
                c->bx.u16 += 1;
                c->ip += 1;
                break;

            case 0x48: /* dec ax */
                c->ax.u16 -= 1;
                c->ip += 1;
                break;

            case 0x50: // push ax
                if (c->sp < 2) {
                    fprintf(stderr, "Stack underflow during PUSH AX\n");
                    return 1;
                }

                c->sp -= 2;
                c->memory[c->sp] = c->ax.u16 & 0xFF;         // Store lower byte of AX
                c->memory[c->sp + 1] = (c->ax.u16 >> 8);     // Store higher byte of AX
                c->ip += 1;
                break;

            case 0x53: /* push bx */
                computer_push(c, c->bx.u16);
                c->ip += 1;
                break;

            case 0x5b: /* pop bx */
                computer_pop(c, &c->bx.u16);
                c->ip += 1;
                break;

            case 0x74: /* jz ? */
                e8 = (s8)c->memory[c->ip+1];
                c->ip += 2;
                if (c->flags & mask_flag_z) {
                    c->ip += e8;
                }
                break;

            case 0x84: /* test ? ? */
                param = c->memory[c->ip+1];

                switch (param) {
                    case 0xc0: /* test al, al */
                        /* TODO */
                        n8 = c->ax.part.al & c->ax.part.al;
                        if (n8 == 0) {
                            computer_set_flag(c, mask_flag_z);
                        }
                        c->ip += 2;
                        break;

                    default:
                        fprintf(stderr, "unknown param 0x%02x\n\n", param);
                        return 1;
                }
                break;

            case 0x8a: /* mov al, [bx] */
                n8 = c->memory[c->bx.u16];
                c->ax.part.al = n8;
                c->ip += 2;
                break;


            case 0x8E: // MOV segment register, r/m16
                {
                    u8 modrm = c->memory[c->ip + 1]; // Fetch ModR/M byte
                    u8 reg = (modrm >> 3) & 0x07;   // Extract `reg` field (segment register)
                    u16 value;

                    // Decode the source operand (r/m16)
                    if ((modrm & 0xC0) == 0xC0) { // Register mode
                        u8 rm = modrm & 0x07;     // Extract `r/m` field (source register)
                        switch (rm) {
                        case 0: value = c->ax.u16; break; // AX
                        case 1: value = c->cx.u16; break; // CX
                        case 2: value = c->dx.u16; break; // DX
                        case 3: value = c->bx.u16; break; // BX
                        case 4: value = c->sp;     break; // SP
                        case 5: value = c->bp;     break; // BP
                        case 6: value = c->si;     break; // SI
                        case 7: value = c->di;     break; // DI
                        default:
                            fprintf(stderr, "Invalid r/m field in opcode 0x8E\n");
                            return 1; // Error
                        }
                    } else {
                        // Memory mode: Decode the effective address (EA)
                        u16 address = decode_effective_address(c, modrm);
                        value = computer_read_u16le(c, address); // Read 16-bit value from memory
                    }

                    // Assign the value to the specified segment register
                    switch (reg) {
                        case 0: c->es = value; break; // ES
                        case 1: c->cs = value; break; // CS
                        case 2: c->ss = value; break; // SS
                        case 3: c->ds = value; break; // DS
                        default:
                            fprintf(stderr, "Invalid segment register in opcode 0x8E\n");
                            return 1; // Error: Invalid segment register
                    }

                    c->ip += 2; // Move to the next instruction (opcode + ModR/M byte)
                }
                break;

            case 0xb0: /* mov al, imm8 */
                n8 = c->memory[c->ip+1];
                c->ax.part.al = n8;
                c->ip += 2;
                break;

            case 0xb4: /* mov ah, ?*/
                n8 = c->memory[c->ip+1];
                c->ax.part.ah = n8;
                c->ip += 2;
                break;

            case 0xb8: /* mov ax, n16*/
                n16 = computer_read_u16le(c, c->ip + 1);
                /* printf("n16 %X\n", n16); */
                c->ax.u16 = n16;
                c->ip += 3;
                break;

            case 0xbb: /* mov bx, n16*/
                n16 = computer_read_u16le(c, c->ip + 1);
                /* printf("n16 %X\n", n16); */
                c->bx.u16 = n16;
                c->ip += 3;
                break;

            case 0xcd: /* int ? */
                n8 = c->memory[c->ip+1];

                if (c->interupt[n8] == NULL) {
                    fprintf(stderr, "unknown interupt 0x%02x\n\n", n8);
                    return 1;
                }

                if (c->interupt[n8](c)) {
                    return 1;
                }

                c->ip += 2;
                break;

            case 0xeb: /* jmp short ? */
                e8 = c->memory[c->ip+1];
                c->ip += 2 + e8;
                break;

            case 0xe5: /* in ax, ?*/
                port = c->memory[c->ip+1];
                c->ax.u16 = computer_read_port(c, port);
                c->ip += 2;
                break;

        default:
            fprintf(stderr, "unknown opcode 0x%02x\n\n", opcode);
            return 1;
        }

    }

    return 0;
}


void
hexdump(u8 *src, int offset, int size) {
    int i = offset;
    int j = 0;
    for (; size > 0; size -= 16) {
        printf("%08x | ", i);
        for (j = 0; j < 16; j += 2) {
            printf("%02x%02x ", *(src + i + j), *(src + i + j+1));
        }

        printf("| ");

        for (j = 0; j < 16; j += 1) {
            printf("%c",
                    *(src + i + j) > ' ' ?
                    *(src + i + j) : '.'
                    );
        }

        printf("\n");

        i += 16;
    }
}


