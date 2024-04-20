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
