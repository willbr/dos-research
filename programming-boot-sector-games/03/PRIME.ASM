   org 0x0100

table:      equ 0x8000
table_size: equ 1000

start:
   mov bx, table
   mov cx, table_size
   mov al, 0

p1:
   mov [bx], al
   inc bx
   loop p1
   mov ax, 2
p2:
   mov bx, table
   add bx, ax
   cmp byte [bx],0
   jne p3
   push ax
   call display_number
   mov al, 0x2c ; comma
   call display_letter
   pop ax
   mov bx, table
   add bx, ax
p4:
   add bx, ax
   cmp bx, table+table_size
   jnc p3
   mov byte [bx], 1
   jmp p4

p3:
   inc ax
   cmp ax, table_size
   jne p2

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
