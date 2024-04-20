
org 100h ; COM files are loaded at 0x100 in memory

section .text
start:
    ; Print the prompt
    mov dx, prompt
    mov ah, 09h
    int 21h

    ; Read input from the user
    lea dx, [buffer]
    mov ah, 0Ah
    int 21h

    ; Echo the input back
    lea dx, [buffer + 2] ; skip the maximum character count and actual count
    mov ah, 09h
    int 21h

    jmp start ; loop indefinitely

section .data
prompt db '>', '$'
buffer db 255, 0, 255 dup(0) ; buffer space for input


