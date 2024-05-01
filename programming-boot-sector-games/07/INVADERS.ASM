

%ifndef com_file
com_file: equ 1
%endif

cpu 8086

base: equ 0xfc80

shots:    equ base+0x00
	; 4 shots, 2 bytes each
	; plus space for an ignored shot

old_time: equ base+0x0c
level:    equ base+0x10
lives:    equ base+0x11
sprites:  equ base+0x12

SHIP_ROW:    equ 0x5c*OFFSET_X
X_WIDTH:     equ 0x0140
OFFSET_X     equ X_WIDTH*2
SPRITE_SIZE: equ 4

SPACESHIP_COLOR:         equ 0x1c
BARRIER_COLOR:           equ 0x0b
SHIP_EXPLOSION_COLOR:    equ 0x0a
INVADER_EXPLOSION_COLOR: equ 0x0e
BULLET_COLOR:            equ 0x0c
START_COLOR:             equ ((sprites+SPRITE_SIZE-(shots+2))/SPRITE_SIZE+0x20)



%if com_file
   org 0x0100
%else
   org 0x7c00
%endif
   mov ax, 0x0013
   int 0x10             ; 320x200x256
   cld
   mov ax, 0xa000
   mov ds, ax
   mov es, ax
   mov ah, 0x04
   mov [level], ax      ; level = 0, lives = 4


restart_game:
   xor ax, ax
   mov cx, level/2
   xor di, di
   rep
   stosw

   mov ax, [di]
   inc ax
   inc ax
   stosw
   mov ah, al
   xchg ax, dx


   mov ax, SPACESHIP_COLOR*0x0100+0x00
   stosw
   mov ax, SHIP_ROW+0x4c*2
   stosw


   mov ax, 0x08*OFFSET_X+0x28
   mov bx, START_COLOR*0x0100+0x10
in1:
   mov cl, 0x0b
in5:
   stosw
   add ax, 0x0b*2
   xchg ax, bx
   stosw
   inc ah
   xchg ax, bx
   loop in5
   add ax, 0x09*OFFSET_X-11*0x000b*2
   cmp bh, START_COLOR+55
   jne in1

   ; draw the barriers

   mov di, 0x55*OFFSET_X+0x10*2
   mov cl, 5


in48:
   mov ax, BARRIER_COLOR*0x0100+0x04
   call draw_sprite
   add di, 0x1e*2
   loop in48

in14:
   mov si, sprites+SPRITE_SIZE

   ; game loop
   ; si = next invader to animate
   ; dl = state 0:left 1:right >=2:down
   ; dh = nstate
   ; ch = dead invaders
   ; bp = frame counter
in46:
   cmp byte [si+2], 0x20
   jc in2
   inc ch
   cmp ch, 55
   je restart_game

   ; yes, speed up invaders

in6:
   lodsw
   xchg ax, di
   lodsw
   cmp al, 0x28
   je in27
   cmp al, 0x20
   jne in29
   mov byte [si-2], 0x28
in29:
   call draw_sprite
in27:
   cmp si, sprites+56*SPRITE_SIZE
   jne in46
   mov al, dh
   sub al, 2
   jc in14
   xor al, 1
   mov dl, al
   mov dh, al
   jmp in14

in2:
   xor byte [si+2], 8
   inc bp
   and bp, 7
   push dx
   push si
   push bp
   jne in12

in22:
   mov ah, 0x00
   int 0x1a
   cmp dx, [old_time]
   je in22
   mov [old_time], dx

in12:
   mov si, shots
   mov cx, 4
   lodsw
   cmp ax, X_WIDTH
   xchg ax, di
   jc in31

   call zero
   sub di, X_WIDTH+2
   mov al, [di]
   sub al, 0x20
   jc in30
   push si
   push di
   mov ah, SPRITE_SIZE
   mul ah
   add si, ax
   lodsw
   xchg ax, di
   mov byte [si], 0x20
   mov ax, INVADER_EXPLOSION_COLOR*0x0100+0x08
   call draw_sprite
   pop di
   pop si
   jmp in31

in24:
   lodsw
   or ax, ax
   je in23
   cmp ax, 0x60*OFFSET_X
   xchg ax, di
   jnc in31
   call zero
   add di, X_WIDTH-2


   ; draw bullet
in30:
   mov ax, BULLET_COLOR*0x0100+BULLET_COLOR
   mov [si-2], di
   cmp byte [di+X_WIDTH], BARRIER_COLOR
   jne in7


   ; remove bullet
in31:
   xor ax, ax
   mov [si-2], ax

in7:
   cmp byte [di], SPACESHIP_COLOR
   jne in41

   mov word [sprites], SHIP_EXPLOSION_COLOR*0x0100+0x38

in41:
   call big_pixel

in23:
   loop in24

   mov si, sprites
   lodsw
   or al, al
   je in42
   add al, 0x08
   jne in42
   mov ah, SPACESHIP_COLOR
   dec byte [lives]
   js in10
in42:
   mov [si-2], ax
   mov di, [si]
   call draw_sprite
   jne in43

   mov ah, 0x02  ; bios get keyboard flags
   int 0x16

%if com_file
   test al, 0x10 ; scroll lock?
   jnz in10
%endif
   
   test al, 0x04 ; ctrl?
   jz in17
   dec di        ; move 2 pixel to the left
   dec di
in17:
   test al, 0x08 ; alt?
   jz in18
   inc di        ; move 2 pixel to the right
   inc di
in18:
   test al, 0x03 ; shift?
   jz in35
   cmp word [shots], 0
   jne in35
   lea ax, [di+(0x04*2)]
   mov [shots], ax
in35:
   xchg ax, di
   cmp ax, SHIP_ROW-2
   je in43
   cmp ax, SHIP_ROW+0x0132
   je in43
in19:
   mov [si], ax
in43:
   pop bp
   pop si
   pop dx

   mov ax, [si]
   cmp dl, 1
   jbe in9
   add ax, 0x0280
   cmp ax, 0x55*0x280
   jc in8
in10:
%if com_file
   mov ax, 0x0003 ; text mode
   int 0x10
   int 0x20       ; exit to dos
%else
   jmp $
%endif

in9:
   dec ax         ; moving let
   dec ax
   jc in20
   add ax, 4      ; moving right
in20:
   push ax
   shr ax, 1
   mov cl, 0xa0
   div cl
   dec ah
   cmp ah, 0x94
   pop ax
   jb in8
   or dh, 22
in8:
   mov [si], ax
   add ax, 0x06*0x280+0x03*2
   xchg ax, bx

   mov cx, 3
   in al, (0x40)
   cmp al, 0xfc
   jc in4
   mov di, shots+2
in45:
   cmp word [di], 0
   je in44
   scasw
   loop in45
in44:
   mov [di], bx
in4:
   jmp in6



bitmaps:
   db 0x18,0x18,0x3c,0x24,0x3c,0x7e,0xff,0x24
   db 0x00,0x80,0x42,0x18,0x10,0x48,0x82,0x01
   db 0x00,0xbd,0xdb,0x7e,0x24,0x3c,0x66,0xc3
   db 0x00,0x3c,0x5a,0xff,0xa5,0x3c,0x66,0x66
   db 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00


bit:
   jc big_pixel
zero:
   xor ax, ax
big_pixel:
   mov [di+X_WIDTH], ax
   stosw
   ret


draw_sprite:
   push cx
   push di
   pushf
in3:
   push ax
   mov bx, bitmaps
   cs xlat

   xchg ax, bx
   mov cx, 10
   clc
in0:
   mov al, bh
   mov ah, bh
   call  bit
   shl bl, 1
   loop in0
   add di, OFFSET_X-20
   pop ax
   inc ax
   test al, 7
   jne in3
   popf
   pop di
   pop cx
   ret

%if com_file
%else
   times 510-($-$$) db 0x4f
   db 0x55,0xaa
%endif

