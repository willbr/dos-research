org 0x0100

key_esc:       equ 0x1b
key_backspace: equ 0x08
key_enter:     equ 0x0d
ascii_bell:    equ 0x07
ascii_lf:      equ 0x0a
ascii_cr:      equ 0x0d

section .data
   welcome_message db 5, "a b c"
   prompt db 2, "; "

section .bss
   line_buffer  resb 0xff
   next_token   resw 1
   token_buffer resb 0xff

section .text

start:
   ;mov si, welcome_message
   ;call puts
   ;call newline
   ;int 0x20

   ;mov si, prompt
   ;call puts

   ;mov di, line_buffer
   ;call readline
   ;call newline

    xor cx, cx
    mov si, welcome_message
    mov di, line_buffer
    lodsb
    mov cl, al
    stosb
    .loop:
    lodsb
    stosb
    loop .loop


   ;mov si, line_buffer
   ;call puts
   ; call newline

    mov word [next_token], line_buffer
    call pluck_token
    ; todo break if empty
    call puts


   int 0x20


   ; di = dst address
   ; cx = line length ; can I remove cx? and use length = (di - pop di)?
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

pluck_token:
    mov si, [next_token]
    lodsb
    xor cx, cx
    mov cl, al          ; read length

    mov di, token_buffer
    push di
    push di
    inc di
    .loop:
    cmp cx, 0
    je .end_loop

    lodsb
    cmp al, ' ' ; is space?
    je .end_loop

    stosb

    loop .loop

    .end_loop:
    pop di
    pop di
    ret

skip_spaces:
    lodsb
    cmp al, ' '
    je skip_spaces
    dec si
    ret

