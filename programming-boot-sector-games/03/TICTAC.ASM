



   org 0x0100

board: equ 0x0300

start:
   mov bx, board
   mov cx, 9
   mov al, '1'

b09:
   mov [bx], al
   inc al
   inc bx
   loop b09

b10:
   call show_board
   call find_line
   ;int 0x20

   call get_movement
   mov byte [bx], 'X'

   call show_board
   call find_line

   call get_movement
   mov byte [bx], 'O'
   jmp b10


get_movement:
   call read_keyboard    ; if 'esc' then exit
   cmp al, 0x1b
   je do_exit
   sub al,0x31           ; al -= '1'
   jc get_movement
   cmp al,0x09           ; al > 9
   jnc get_movement
   cbw                   ; Expand AL to 16 bits using AH
   mov bx, board
   add bx, ax
   mov al, [bx]
   cmp al, 0x40
   jnc get_movement
   call show_crlf
   ret

do_exit:
   int 0x20


find_line:
   mov al, [board]
   cmp al, [board+1]
   jne b01
   cmp al, [board+2]
   je  won
b01:
   cmp al, [board+3]
   jne b04
   cmp al, [board+6]
   je won
b04:
   cmp al, [board+4]
   jne b05
   cmp al, [board+8]
   je won
b05:
   mov al, [board+3]
   cmp al, [board+4]
   jne b02
   cmp al, [board+5]
   je won

b02:
   mov al, [board+6]
   cmp al, [board+7]
   jne b03
   cmp al, [board+8]
   je won
b03:
   mov al, [board+1]
   cmp al, [board+4]
   jne b06
   cmp al, [board+7]
   je won
b06:
   mov al, [board+2]
   cmp al, [board+5]
   jne b07
   cmp al, [board+8]
   je won
b07:
   cmp al, [board+4]
   jne b08
   cmp al, [board+6]
   je won
b08:
   ret

won:
   call display_letter
   
   mov al, 0x20 ; ' '
   call display_letter
   mov al, 0x77 ; 'w'
   call display_letter
   mov al, 0x69 ; 'i'
   call display_letter
   mov al, 0x6e ; 'n' 
   call display_letter
   mov al, 0x73 ; 's'
   call display_letter
   int 0x20


show_board:
   mov bx, board
   call show_row
   call show_div
   mov bx, board+3
   call show_row
   call show_div
   mov bx, board+6
   jmp show_row

show_row:
   call show_square
   mov al, 0x7c
   call display_letter
   call show_square
   mov al, 0x7c
   call display_letter
   call show_square

show_crlf:
   mov al, 0x0d
   call display_letter
   mov al, 0x0a
   jmp display_letter

show_div:
   mov al, 0x2d
   call display_letter
   mov al, 0x2b
   call display_letter
   mov al, 0x2d
   call display_letter
   mov al, 0x2b
   call display_letter
   mov al, 0x2d
   call display_letter
   jmp show_crlf

show_square:
   mov al, [bx]
   inc bx
   jmp display_letter
   ; library1.asm

   int 0x20

 display_letter:
   push ax
   push bx
   push cx
   push dx
   push si
   push di
   mov ah,0x0e      ; terminal output
   mov bx, 0x000f   ; graphics mode
   int 0x10         ; call the BIOS for displaying one letter
   pop di
   pop si
   pop dx
   pop cx
   pop bx
   pop ax
   ret


    ; read keyboard into AL
 read_keyboard:
     push bx
     push cx
     push dx
     push si
     push di
     mov ah,0x00       
     int 0x16          ; BIOS for reading keyboard
     pop di
     pop si
     pop dx
     pop cx
     pop bx
     ret


display_number:
   mov dx, 0
   mov cx, 10
   div cx
   push dx
   cmp ax,0
   je display_number_1
   call display_number
display_number_1:
   pop ax
   add al,'0'
   call display_letter
   ret
