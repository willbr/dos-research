
 cpu 8086

 org 0x0100

 v_a: equ 0xfa00
 v_b: equ 0xfa02

 start:
   mov ax, 0x0013    ; video mode 320x200
   int 0x10

   mov ax, 0xa000    ; segment address for vga memory 
   mov ds, ax
   mov es, ax

m4:
   mov ax, 127
   mov [v_a], ax
m0:
   mov ax, 127
   mov [v_b], ax

m1:
   mov ax, [v_a]
   mov dx, 320
   mul dx
   add ax, [v_b]
   xchg ax, di

   mov ax, [v_a]
   and ax, 0x78
   add ax, ax

   mov bx, [v_b]
   and bx, 0x78
   mov cl, 3
   shr bx, cl
   add ax, bx
   stosb

   dec word [v_b]
   jns m1

   dec word [v_a]
   jns m0

   mov ah, 0x00
   int 0x16

   mov ax, 0x0002      ; 80x25 text mode
   int 0x10

   int 0x20            ; exit


