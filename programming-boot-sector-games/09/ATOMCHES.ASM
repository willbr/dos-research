; command format d2d4

use16

%ifndef com_file
com_file: equ 1
%endif

%if com_file
   org 0x0100
%else
   org 0x7c00
%endif



   cld

%if com_file
   ;
%else
   mov sp, stack
   push cs
   push cs
   push cs
   pop ds
   pop es
   pop ss
%endif
   ; create board
   mov di, board-8
   mov cx, di
sr1:
   push di
   pop ax
   and al, 0x88 ; 0x88 board
   jz sr2
   mov al, 0x07 ; Frontier
sr2:
   stosb
   loop sr1

   ; setup board
   mov si, initial
   mov di, board
   mov cl, 0x08
sr3:
   lodsb              ; load piece
   stosb              ; black piece
   or al, 8
   mov [di+0x6f], al  ; white pieces
   inc byte [di+0x0f]         ; black pawn
   mov byte [di+0x5f], 0x09   ; white pawn
   loop sr3

sr21:
   push sr21    ; 7th repeat loop
   push play    ; 6th computer play ch: 8=white, 0=black
   push display_board    ; 5th returns cx to zero
   push sr28             ; 4th make movement
   mov si, key2
   push si               ; 3rd take coordinate
   push si               ; 2nd take coordinate
display_board:
   mov si, board-8
   			 ; assume ch=0
			 ; loop if play is called with ch=8
   mov cl, 73            ; 1 frontier + 8 rows * (* cols+1 frontier)
sr4:
   lodsb
   mov bx, chars         ; Note BH is reused outside this subroutine
   xlatb
   cmp al, 0x0d          ; Is it BC?
   jnz sr5               ; No, jump
   add si, 7             ; Jump 7 frontier bytes
   call display          ; Display RC
   mov al, 0x0a          ; Now display LF
   call display
sr5:
   call display
   loop sr4
   ret                   ; cx=0
sr14:
   inc dx                ; Shorter than inc dl and doesn't overflow
   dec dh
   jnz sr12
sr17:
   inc si
sr6:
   cmp si, board+120
   jne sr7
   pop di
   pop si
   test cl, cl           ; Top call?
   jne sr24
   cmp bp, -127          ; Illegal move? (any move = always in check)
   jl sr24               ; Yes, don't move
sr28:
   movsb                 ; Do move
   mov byte [si-1], 0    ; Clear origin square
sr24:
   ret
play:
   mov bp, -256          ; Current score
   push bp               ; Origin square
   push bp               ; Target square
   mov si, board
sr7:
   lodsb                 ; Read square
   xor al, ch            ; XOR with current playing side
   dec ax                ; Empty square 0x00 becomes 0xff
   cmp al, 6             ; Ignore if frontier or empty
   jnc sr6
   or al, al             ; Is it a pawn?
   jnz sr8
   or ch, ch             ; Current side is black?
   jnz sr25              ; No, jump
sr8:
   inc ax
sr25:
   dec si
   add al, 0x04
   mov ah, al            ; Total movements of piece in ah (later dh)
   and ah, 0x0c
   mov bl, offsets-4
   xlatb
   xchg dx, ax           ; Movements offset in dl
sr12:
   mov di, si            ; Restart target square
sr9:
   mov bl, dl            ; Build index into directions
   xchg ax, di
   add al, [bx]          ; Next target square
   xchg ax, di
   mov al, [di]          ; Content of target square in al
   inc ax
   mov ah, [si]          ; Content of origin square in ah
   cmp dl, 16+displacement
   dec al                ; Check for empty square in z flag
   jz sr10               ; Goes to empty square, jump
   jc sr27               ; If not pawn, jump
   cmp dh, 3             ; Straight?
   jb sr17               ; Yes, avoid+cancel any 2 square movement
sr27:
   xor al, ch
   sub al, 0x09          ; Valid capture?
   cmp al, 0x05          ; Check Z with king, c=0 if invalid
   mov al, [di]
   ja sr18               ; No, avoid
   ; z=0/1 if king captured
   jne sr20              ; Wizard trick, jump if not captured king
   dec cl                ; If not in first response...
   mov bp, 78            ; ...maximum score
   jne sr26
   add bp, bp            ; Maximum score (maybe checkmate/stalemate)
sr26:
   pop ax                ; Ignore values
   pop ax
   ret
sr20:
   push ax               ; Save for restoring in near future
   and al, 7
   mov bl, scores
   xlatb
   cbw
   ; cmp cl, 4            ; 4-ply depth
   ; cmp cl, 3            ; 3-ply depth
   ; cmp cl, 2            ; 2-ply depth
   cmp cl, 1             ; 1-ply depth
   jnc sr22
   pusha                 ; Save all state (+ current side in ch)
   call sr28             ; Do move
   xor ch, 8             ; Change side
   inc cx                ; Increase depth
   call play
   mov bx, sp
   sub [bx+14], bp       ; Subtrack BP from AX
   popa                  ; Restore state
sr22:
   cmp bp, ax            ; Better score?
   jg sr23               ; No, jump
   xchg ax, bp           ; New best score
   jne sr23              ; Same score?
   in al, (0x40)
   cmp al, 0xaa          ; Randomize it
sr23:
   pop ax                ; Restore board
   mov [si], ah
   mov [di], al
   jg sr18
   add sp, 4
   push si               ; Save movement
   push di
sr18:
   dec ah
   xor ah, ch            ; Was it a pawn?
   jz sr16               ; Yes, end sequence, choose next movement
   cmp ah, 0x04          ; Knight or king?
   jnc sr16              ; End sequence, choose next movement
   or al, al             ; To empty square?
   jz sr9                ; Yes, follow line of squares
sr16:
   jmp sr14
sr10:
   jc sr20               ; If no pawn, jump
   cmp dh, 2             ; Diagonal?
   ja sr18               ; Yes, avoid
   jnz short sr20        ; Advances one square? No, jump
   xchg ax, si
   push ax
   sub al, 0x20
   cmp al, 0x40          ; Moving from the center of the board?
   pop ax
   xchg ax, si
   sbb dh, al            ; Yes, then avoid checking for two squares
   jmp short sr20

   ; Read algebraic coordinate
key2:
   xchg si, di
   call key              ; Read letter
   xchg di, ax
   			 ; Fall through to read number
   ; Read a key and display it
key:
   mov ah, 0             ; Read keyboard
   int 0x16              ; Call BIOS, only affects AX and flags

display:
   pusha
   mov ah, 0x0e          ; Console output
   mov bh, 0x00
   int 0x10              ; Call BIOS, can affect AX in old VGA BIOS
   popa
   and ax, 0x0f          ; Extract column
   imul bp, ax,-0x10     ; Calculate digit row multiplied by 16
   lea di,[bp+di+board+127] ; Subtract board column
   ret

initial:
   db 2, 5, 3, 4, 6, 3, 5, 2
scores:
   db 0, 1, 5, 3, 9, 3
chars:
   db ".prbqnk", 0x0d, ".PRBQNK"
offsets:
   db 16+displacement
   db 20+displacement
   db  8+displacement
   db 12+displacement
   db  8+displacement
   db  0+displacement
   db  8+displacement
displacement:
   db -33, -31, -18, -14, 14, 18, 31, 33
   db -16, 16, -1, 1
   db 15, 17, -15, -17
   db -15, -17, -16, -32
   db 15, 17, 16, 32


%if com_file
board: equ 0x0300
%else
   db "Toledo Atomchess. Mar/04/2016"
   db " (c) 2015-2016 Oscar Toledo G. "
   db "www.nanochess.org"
   db " Happy coding! :-) "
   db "Most fun MBR ever!!"
   db 0, 0, 0

   db 0x55, 0xaa

   board: equ 0x7e00
   stack: equ 0x8000
%endif

