org 0x0100

key_esc:       equ 0x1b
key_backspace: equ 0x08
key_enter:     equ 0x0d
ascii_bell:    equ 0x07
ascii_lf:      equ 0x0a
ascii_cr:      equ 0x0d

start:
   mov ah, 0x00
   int 0x16          ; read key

   cmp al, key_esc
   je exit

   cmp al, key_backspace
   jne not_backspace

handle_backspace:
   call putchar
   mov al, ' '
   call putchar
   mov al, key_backspace
   call putchar
   jmp start

not_backspace:
   cmp al, key_enter
   jne not_enter

   mov al, ascii_lf
   call putchar
   mov al, ascii_cr

not_enter:
   call putchar
   jmp start

putchar:
   mov ah, 0x0e
   mov bx, 0x000f
   int 0x10
   ret



exit:
   int 0x20

