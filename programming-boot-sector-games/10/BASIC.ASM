; bootBASIC interpreter

; by Oscar Toledo G
; http://nanochess.org/

; (c) Copyright 2019 Oscar Toledo G


cpu 8086

%ifndef com_file
com_file: equ 1
%endif

%if com_file
org 0x0100
%else
org 0x7c00
%endif

vars:       equ 0x7e00 ; Variables (multiple of 256)
running:    equ 0x7e7e ; Running status
line:       equ 0x7e80 ; line input
program:    equ 0x7f00 ; Program address
stack:      equ 0xff00 ; Stack address
max_line:   equ 1000 ; First unavailable line number
max_length: equ 20   ; Maximum length of line
max_size:   equ max_line*max_length ; Max program size

start:
%if com_file
%else
    push cs
    push cs
    push cs
    pop ds
    pop es
    pop ss
%endif
   cld
   mov di, program
   mov al, 0x0d          ; Fill with '\n'
   mov cx, max_size
   rep stosb             ; initialize



main_loop:
   mov sp, stack
   xor ax, ax
   mov [running], ax
   mov al, '>'           ; prompt char
   call input_line       ; read line
   call input_number     ; get number
   or ax, ax             ; no number or zero?
   je f14
   call find_line
   xchg ax, di
   mov cx, max_length
   rep movsb             ; copy entered line into program
   jmp main_loop

f14:
   call statement
   jmp main_loop



if_statement:
   call expr           ; eval expr
   or ax, ax           ; is zero?
   je f6

 statement:
   call spaces          ; avoid spaces
   cmp byte [si], 0x0d  ; empty line?
   je f6
   mov di, statements
f5:
   mov cl, [di]         ; return length of the string
   mov ch, 0
   test cx, cx
   je f4
   push si              ; save current position
   inc di
f16:
   rep cmpsb
   jne f3
   pop ax
   call spaces
   jmp word [di]

f3:
   add di, cx           ; advance the list pointer
   inc di
   inc di
   pop si
   jmp f5

f4:
   call get_variable
   push ax              ; save address
   lodsb                ; read a line letter
   cmp al, '='
   je assignment



error:
   mov si, error_message
   call print_2         ; display message
   jmp main_loop

error_message:
   db "@#!", 0x0d

list_statement:
   xor ax, ax
f29:
   push ax
   call find_line
   xchg ax, si
   cmp byte [si], 0x0d
   je f30
   pop ax
   push ax
   call output_number      ; show line number
f32:
   lodsb
   call output
   cmp al, 0x0d
   jne f32
f30:
   pop ax
   inc ax
   cmp ax, max_line
   jne f29
f6:
   ret


input_statement:
   call get_variable
   push ax
   mov al, '?'
   call input_line

    ; second part of the assignment statement


assignment:
   call expr
   pop di
   stosw             ; save on the variable
   ret


expr:
   call expr1
f20:
   cmp byte [si], '-'
   je f19
   cmp byte [si], '+'
   jne f6
   push ax
   call expr1_2
   pop cx
   add ax, cx
   jmp f20


f19:
   push ax
   call expr1_2
   pop cx
   xchg ax, cx
   sub ax, cx
   jmp f20


   ; second tier: division & multiplication

expr1_2:
   inc si
expr1:
   call expr2
f21:
   cmp byte [si], '/'
   je f23
   cmp byte [si], '*'
   jne f6

   push ax
   call expr2_2
   pop cx
   imul cx
   jmp f21


f23:
   push ax
   call expr2_2
   pop cx
   xchg ax, cx
   cwd
   idiv cx
   jmp f21


   ; Third tier: parentheses, numbers and vars

expr2_2:
   inc si             ; avoid the operator
expr2:
   call spaces        ; skip spaces
   lodsb              ; read char
   cmp al, '('
   jne f24
   call expr
   cmp byte [si], ')'
   jne error
   jmp spaces_2
f24:
   cmp al, 0x40       ; variable?
   jnc f25
   dec si             ; back one letter
   call input_number
   jmp spaces
f25:
   call get_variable_2
   xchg ax, bx
   mov ax, [bx]
   ret


get_variable:
   lodsb
get_variable_2:
   and al, 0x1f         ; 0x61-0x71 -> 0x01-0x1a
   add al, al           ; x 2 (each variable = word)
   mov ah, vars>>8      ; setup high byte of address

   ; avoid spaces

spaces:
   cmp byte [si], ' '
   jne f22

   ; avoid spaces after current character

spaces_2:
   inc si
   jmp spaces


   ; output unsigned number
   ; ax = value
   ; 
output_number:
f26:
    xor dx, dx        ; dx:ax
   mov cx, 10         ; divisor = 10
   div cx
   or ax, ax         ; at left?
   push dx
   je f8
   call f26          ; output left side
f8:
   pop ax
   add al, '0'       ; output remainder as ascii digit
   jmp output



   ; read number in input
   ; ax = result
   ;
input_number:
   xor bx, bx
f11:
   lodsb
   sub al, '0'
   cmp al, 10             ; valid digit?
   cbw
   xchg ax, bx
   jnc f12
   mov cx, 10
   mul cx                 ; multiply by 10
   add bx, ax             ; add new digit
   jmp f11
f12:
   dec si                 ; si points to first non-digit
f22:
   ret

   ; handle 'run' statement
   ; (requivalent to 'goto 0')

run_statement:
    xor ax, ax
   jmp f10


   ; handle 'goto'
goto_statement:
   call expr
f10:
   call find_line
f27:
   cmp word [running], 0
   je f31
   mov [running], ax
   ret
f31:
   push ax
   pop si
   add ax, max_length         ; points to next line
   mov [running], ax          ; save for next time
   call statement
   mov ax, [running]
   cmp ax, program+max_size   ; reach the end?
   jne f31
   ret



   ; find line in program
   ; entry:
   ;    ax = line number
   ; result:
   ;    ax = pointer to program
find_line:
   mov cx, max_length
   mul cx,
   add ax, program
   ret


   ; input line from keyboard
   ; entry:
   ;    al = prompt character
   ; result
   ;    buffer 'line' contains line, finished with '\r'
   ;    si points to 'line'

input_line:
   call output
   mov si, line
   push si
   pop di
f1:
   call input_key
   cmp al, 0x08       ; backspace?
   jne f2
   dec di
   jmp f1
f2:
   stosb              ; save key
   cmp al, 0x0d       ; CR pressed?
   jne f1
   ret

   ; handle 'print' statement
print_statement:
   lodsb
   cmp al, 0x0d
   je new_line
   cmp al, '"'
   jne f7
print_2:
f9:
   lodsb
   cmp al, '"'
   je f18
   call output
   cmp al, 0x0d
   jne f9
   ret
f7:
   dec si
   call expr
   call output_number
f18:
   lodsb
   cmp al, ';'
   jne new_line
   ret


   ; read a key into al
   ; also outputs it to screen
input_key:
   mov ah, 0x00
   int 0x16
    ;
   ; screen output of character contained in al
   ; expands 0x0d '\n' intro 0x0a 0x0d '\r' '\n'
    ;

output:
    cmp al, 0x0d
   jne f17

   ; go to next line (generates \r\n)
new_line:
   mov al, 0x0a
   call f17
   mov al, 0x0d
f17:
   mov ah, 0x0e
   int 0x10
   ret

   ; list of statements of bootBASIC
   ; First one byte with length of string
   ; Then string with statement
   ; Then a word with the address of the code

statements:
   db 3, "new"
   dw start

   db 4, "list"
    dw list_statement

   db 3, "run"
   dw run_statement

   db 5, "print"
   dw print_statement

   db 5, "input"
   dw input_statement

   db 2, "if"
   dw if_statement

   db 4, "goto"
   dw goto_statement

   %if com_file
   db 6, "system"
   dw 0                 ; location 0 contains int 0x20 (setup by DOS)
   %endif

   db 0

   ; boot sector filler
   %if com_file
   %else
   times 510-($-$$) db 0x4f
   db 0x55, 0xaa
   %endif

