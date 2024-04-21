org 0x0100

mov ax, 0x0002
int 0x10                ; set video mode to text

mov ax, 0xb800          ; video memory in text mode
mov ds, ax
mov es, ax


main_loop:
   mov  ah, 0x00
   int  0x1a            ; get time

   mov  al, dl          ; dl = 1/100 seconds
   test al, 0x40
   je   m2

   not al

m2:
   and al, 0x3f
   sub al, 0x20
   cbw                  ; signed convert byte to word ax = al
   mov cx, ax

   mov di, 0x0000
   mov dh, 0

m0:
   mov dl, 0

m1:
   push dx
   mov bx, sin_table

   mov al, dh
   shl al, 1            ; shift left by 1
   and al, 0x3f
                        ; sine lookup
   cs xlat              ; al = CS:[BX+AL]
   cbw                  ; ax = al
   push ax

   mov al,dl
   and al,0x3f
   cs xlat              ; same again, sine lookup
   cbw

   pop dx
   add ax, dx
   add ax, cx
   mov ah, al
   mov al, 0x2a         ; char '*'
   mov [di], ax
   add di, 2

                        ; loop for columns
   pop dx
   inc dl
   cmp dl, 80
   jne m1

                        ; loop for rows
   inc dh
   cmp dh, 25
   jne m0

   mov ah, 0x01
   int 0x16          ; check for key press without waiting

   jne key_pressed
   jmp main_loop

key_pressed:
   int 0x20          ; exit




sin_table:
   db 0,6,12,19,24,30,36,41
   db 45, 49,53,56,59,61,63,64
   db 64,64,63,61,59,56,53,49
   db 45,41,36,30,24,19,12,6
   db 0,-6,-12,-19,-24,-30,-36,-41
   db -45,-49,-53,-56,-59,-61,-63,-64
   db -64,-64,-63,-61,-59,-56,-53,-49
   db -45,-41,-36,-30,-24,-19,-12,-6
   
