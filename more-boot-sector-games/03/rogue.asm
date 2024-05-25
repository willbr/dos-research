; boot rouge
; by Oscar Toledo G


cpu 8086

row_width:      equ 0x00a0
box_max_width:  equ 23
box_max_height: equ 6
box_width:      equ 26
box_height:     equ 8

light_color: equ 0x06
hero_color:  equ 0x0e

gr_vert:       equ 0xba
gr_top_right:  equ 0xbb
gr_bot_right:  equ 0xbc
gr_bot_left:   equ 0xc8
gr_top_left:   equ 0xc9
gr_horiz:      equ 0xcd

gr_tunnel: equ 0xb1
gr_door:   equ 0xce
gr_floor:  equ 0xfa

gr_hero:   equ 0x01

gr_ladder: equ 0xf0
gr_trap:   equ 0x04
gr_food:   equ 0x05
gr_armor:  equ 0x08
gr_yendor: equ 0x0c
gr_gold:   equ 0x0f
gr_weapon: equ 0x18

yendor_level: equ 26



com_file: equ 0x01

%ifdef com_file
	org 0x0100
%else
	org 0x7c00
%endif

rnd:    equ 0x0008
starve: equ 0x0006
hp:     equ 0x0004
level:  equ 0x0003
yendor: equ 0x0002
armor:  equ 0x0001
weapon: equ 0x0000



start:
	in ax, 0x40
	push ax
	mov ax, 16
	push ax
	push ax
	mov al, 1
	push ax
	push ax
	inc ax
	int 0x10
	mov ax, 0xb800
	mov ds, ax
	mov es, ax

	mov si, random
	call si

	mov bp, sp

generate_dungeon:
	mov bl, [bp+yendor]
	add [bp+level], bl
%ifdef com_file
	jne .0
	jmp quit
.0:
%else
	je $
%endif

; select a maze for the dungeon

	mov ax, [bp+rnd]
	and ax, 0x4182
	or ax, 0x1a6d
	xchg ax, dx

	; clear the screen to black over black
	xor ax, ax
	xor di, di
	mov ch, 0x08
	;rep stosw


	; draw the nine rooms
.7:
	push ax
	call fill_room
	pop ax
	add ax, box_width*2
	cmp al, 0x9c
	jne .6

	add ax, row_width*box_height-box_width*3*2
.6:
	cmp ax, row_width*box_height*3
	jb .7

	; put the ladder at a random corner room
	shl word [bp+rnd], 1
	mov ax, 3*row_width+12*2
	mov bx, 19*row_width+12*2
	jnc .2
	xchg ax, bx
.2:
	jns .8
	add ax, box_width*2*2
.8:
	xchg ax, di
	mov byte [di], gr_ladder


	; put the amulet of yendor

	cmp byte [bp+level], yendor_level
	jb .1
	mov byte [bx], gr_yendor


.1:
	; setup hero start

	mov di, 11*row_width+38*2

	; main game loop

game_loop:
	mov ax, game_loop
	push ax


	; circle of light
	mov bx, 0x0005
.1:
	dec bx
	dec bx
	mov al, light_color
	mov [bx+di-row_width], al
	mov [bx+di], al
	mov [bx+di+row_width], al
	jns .1

	; show our hero
	push word [di]
	mov word [di], hero_color*256+gr_hero
	add byte [bp+starve], 2
	sbb ax, ax
	call add_hp
	;mov ah, 0x00
	int 0x16
	pop word [di]

	mov al, ah

%ifdef com_file
	cmp al, 0x01
	je quit
%endif

	sub al, 0x4c
	mov ah, 0x02
	cmp al, 0xff
	je .2
	cmp al, 0x01
	je .2
	cmp al, 0xfc
	je .3
	cmp al, 0x04
	jne move_cancel
.3:
	mov ah, 0x28
.2:
	imul ah

	xchg ax, bx
	mov al, [di+bx]


	cmp al, gr_ladder
	je ladder_found

	cmp al, gr_door
	jnc .4

	cmp al, gr_tunnel
	ja move_cancel
.4:
	cmp al, gr_trap
	jb move_cancel
	; move player
	lea di, [di+bx]
	mov bh, 0x06
	je trap_found


	cmp al, gr_tunnel
	jnc move_cancel

	cmp al, gr_weapon
	ja battle
	; only items at this part of code, so clean floor
	mov byte [di], gr_floor

	je weapon_found

	; 4 things to look for
	cmp al, gr_armor
	je armor_found
	jb food_found
	; 2 things to look for
	cmp al, gr_gold
	je move_cancel
	; at this point 'al' can only be gr_yendor
	; Amulet of Yendor found!
	neg byte [bp+yendor]
move_cancel:
	ret


	%ifdef com_file
quit:
	int 0x20
	%endif

ladder_found:
	jmp generate_dungeon

armor_found:
	inc byte [bp+armor]
	ret

weapon_found:
	inc byte [bp+weapon]
	ret

	; Aaaarghhhh!
trap_found:
	call si
sub_hp:
	neg ax
	db 0xbb

food_found:
	call si
add_hp:
	add ax, [bp+hp]
%ifdef com_file
	js quit
%else
	js $
%endif
	mov [bp+hp], ax


	; update screen indicator

	mov bx, 0xf98
	call .1
%ifdef com_file
	mov al, [bp+weapon]
	call .1
	mov al, [bp+armor]
	call .1
%endif

	mov al, [bp+level]
.1:
	xor cx, cx
.2:
	inc cx
	sub ax, 10
	jnc .2
	add ax, 0x0a3a
	call .3
	xchg ax, cx
	dec ax
	jnz .1

.3:
	mov [bx], ax
	dec bx
	dec bx
	ret

battle:
	and al, 0x1f
	shl al, 1
	mov ah, al
	xchg ax, dx

	; Player's attack
.2:
	mov bh, [bp+weapon]
	call si
	sub dh, al
	jc .3

	; Monster's attack
	mov bh, dl
	call si
	sub al, [bp+armor]
	jc .4
	call sub_hp
.4:
	; mov ah, 0x00
	int 0x16
	jmp .2

	; monster is dead
.3:
	mov byte [di], gr_floor
	ret

fill_room:
	add ax, (box_height/2-1)*row_width+(box_width/2)*2
	push ax
	xchg ax, di
	shr dx, 1
	mov ax, 0x0000+gr_tunnel
	mov cx, box_width
	jnc .3
	push di
	;rep stosw
	pop di
.3:

	shr dx, 1
	jnc .5
	mov cl, box_height
.4:
	stosb
	add di, row_width-1
	loop .4

.5:
	mov bh, box_max_width - 2
	call si
	xchg ax, cx
	mov bh, box_max_height - 2
	call si
	mov ch, al
	shr al, 1
	inc ax
	mov ah, row_width
	mul ah
	add ax, cx
	sub ah, ch
	and al, 0xfe
	add al, 0x04
	pop di
	sub di, ax

	mov al, gr_top_left
	mov bx, gr_top_right * 256 + gr_horiz
	call fill
.9:
	mov al, gr_vert
	mov bx, gr_vert * 256 + gr_floor
	call fill
	dec ch
	jns .9
	mov al, gr_bot_left
	mov bx, gr_bot_right * 256 + gr_horiz

	; fill a row on screen for a room
fill:
	push cx
	push di
	call door
.1:
	mov al, bl
	call door
	dec cl
	jns .1
	mov al, bh
	call door
	pop di
	pop cx
	add di, 0x00a0
	ret

	; draw a room character on screen
door:
	cmp al, gr_floor
	jne .3
	call si
	cmp al, 6
	jnc .11
	add al, [bp+level]
.9:
	sub al, 0x05
	cmp al, 0x17
	jge .9
	add al, 0x44
	jmp short .12

.11:
	cmp al, 11
	xchg ax, bx
	cs mov bl, [si+bx+(items-random-6)]
	xchg ax, bx
	jb .12
	mov al, gr_floor
.12:

.3:
	cmp al, gr_horiz
	je .1
	cmp al, gr_vert
	jne .2
.1:
	cmp byte [di], gr_tunnel
	jne .2
	mov al, gr_door
.2:
	stosb
	inc di
	ret

random:
	mov al, 251
	mul byte [bp+rnd]
	add al, 83
	mov [bp+rnd], ax

	; rdtsc ; would make it dependent on Pentium II
	; in al, (0x40) ; Only works for slow requirements

	xor ah, ah
	div bh
	mov al, ah
	cbw
	inc ax,
	ret


items:
	db gr_food
	db gr_gold
	db gr_trap
	db gr_weapon
	db gr_armor

%ifdef com_file
%else
	;times 510-($-$$) db 0x4f
	;db 0x55, 0xaa                ; make it a bootable sector
%endif

