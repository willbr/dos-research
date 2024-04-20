
org 100h ; Standard origin for .COM files

section .text
start:
    ; Set video mode to 03 (80x25 text mode, color)
    mov ax, 0003h
    int 10h

    ; Print colored "Hello, World!" where 1Eh is the color attribute (yellow on blue)
    mov ah, 09h
    mov bl, 1Eh  ; attribute byte (yellow on blue background)
    mov cx, 1    ; number of times to print the character

    ; Print each character with color and move cursor
    mov al, 'H'
    call print_colored
    mov al, 'e'
    call print_colored
    mov al, 'l'
    call print_colored
    mov al, 'l'
    call print_colored
    mov al, 'o'
    call print_colored
    mov al, ','
    call print_colored
    mov al, ' '
    call print_colored
    mov al, 'W'
    call print_colored
    mov al, 'o'
    call print_colored
    mov al, 'r'
    call print_colored
    mov al, 'l'
    call print_colored
    mov al, 'd'
    call print_colored
    mov al, '!'
    call print_colored

    ; Wait for any key press
    mov ah, 00h
    int 16h

    ; Return to DOS
    mov ax, 4C00h
    int 21h

; Subroutine to print a character in color and move the cursor
print_colored:
    int 10h         ; print character in color
    inc dl          ; move cursor right
    mov ah, 02h     ; set cursor position function
    int 10h         ; apply cursor move
    ret

section .bss
    cursor_pos resb 2

section .data
