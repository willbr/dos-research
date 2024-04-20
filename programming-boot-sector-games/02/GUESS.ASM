   org 0x0100

   in al,(0x40)
   and al,0x07
   add al,0x30
   mov cl,al
game_loop:
   mov al,'?'
   call display_letter
   call read_keyboard
   cmp al, cl
   jne game_loop
   call display_letter
   mov al,':'
   call display_letter
   mov al, ')'
   call display_letter

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
