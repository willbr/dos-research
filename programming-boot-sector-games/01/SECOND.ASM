
    ; The incredible keyboard reading program

    org 0x0100

start:
    
    mov ah, 0x00        ; keyboard read
    int 0x16            ; call the bios to read it

    cmp al,0x1b         ; is it ESC?
    je exit_to_command_line

    mov ah, 0x0e
    mov bx, 0x000f
    int 0x10            ; call the bios for displaying one letter

    jmp start

exit_to_command_line:
    int 0x20
    
