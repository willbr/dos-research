org 0x0100

mov ax, 0x0002
int 0x10              ; set cursor

mov ax, 0xb800        ; text video memory
mov ds, ax
mov es, ax
cld                   ; clear direction flag

                      ; text data is a word per char
		      ; first byte is ascii code
		      ; second is colour

xor di, di            ; zero di offset

mov ax, 0x1a48        ; 'H'
stosw                 ; store word and increment di

mov ax, 0x1b45        ; 'E'
stosw

mov ax, 0x1c4c        ; 'L'
stosw

mov ax, 0x1d4c        ; 'L'
stosw

mov ax, 0x1e4f        ; 'O'
stosw

int 0x20              ; terminate

