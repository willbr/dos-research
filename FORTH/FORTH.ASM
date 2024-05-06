org 0x0100

key_esc:       equ 0x1b
key_backspace: equ 0x08
key_enter:     equ 0x0d
ascii_bell:    equ 0x07
ascii_lf:      equ 0x0a
ascii_cr:      equ 0x0d

section .data
   welcome_message db 5, "hello"
   prompt db 2, "; "

section .bss
   line_buffer resb 0xff

section .text

start:
   mov si, welcome_message
   call puts
   call newline
   ;int 0x20

   mov si, prompt
   call puts

   mov di, line_buffer
   call readline

   call newline

   mov si, line_buffer
   call puts

   int 0x20


   ; di = dst address
   ; cx = line length ; can I remove cx? and use length = (si - pop si)?
readline:
   mov cx, 0
   push di
   inc di

.loop:
   mov ah, 0x00
   int 0x16              ; read key

   cmp al, key_esc       ; if esc return
   je .calc_length

   cmp al, key_backspace
   jne .other_chars

   cmp cx, 0
   je .underflow

   ; todo check for overflow

   dec cx
   dec di

   call putchar
   mov al, ' '
   call putchar
   mov al, key_backspace
   call putchar
   jmp .loop

.other_chars:
   inc cx
   stosb

   cmp al, key_enter
   je .calc_length

   call putchar
   jmp .loop

.underflow:
   mov al, ascii_bell
   call putchar
   mov al, key_backspace
   call putchar
   jmp .loop

.calc_length:
   pop di
   mov [di], cl
   ret

putchar:
   mov ah, 0x0e
   mov bx, 0x000f
   int 0x10
   ret

puts:
   xor cx, cx
   mov cl, [si] ; read string length
   inc si

   mov ah, 0x0e
   mov bx, 0x000f
.puts_loop:
   lodsb
   int 0x10
   loop .puts_loop
   ret


newline:
   mov al, ascii_lf
   call putchar
   mov al, ascii_cr
   call putchar
   ret

