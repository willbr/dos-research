; follow the lights
; by Oscar Toledo G

cpu 8086

com_file: equ 0x01

%ifdef com_file
org 0x0100
%else
org 0x7c00
%endif

old_time:   equ 0x00 ; old ticks
button:     equ 0x02 ; button pressed
next_seq:   equ 0x04
timing:     equ 0x06 ; current timing
memory:     equ 0x08 ; start of memory
memory_end: equ 0x28 ; end of memory


start:
	xor ax, ax
	mov cx, memory_end/2
.0:
	push ax
	loop .0

	mov al, 0x02
	int 0x10        ; text mode 80x25

	mov bp, sp

	in al, 0x40 ; get a pseudo random number
	xchg ax, si

	cld
	mov ax, 0xb800     ; video segment
	mov ds, ax
	mov es, ax

	call show_buttons

restart_game:
	xor ax, ax
	mov [bp+next_seq], ax

game_loop:
	mov cl, 15
	call wait_ticks

	; add a new light to sequence

	mov di, [bp+next_seq]

	mov ax, 97
	mul si
	add ax, 23
	xchg ax, si

	and ah, 0x03                      ; extract random from high byte
	add ah, 0x31                      ; add ascii '1'
	mov [bp+di+memory], ah

	mov ax, 8
	cmp di, 5                         ; if number of lights < 5
	jb .2
	mov al, 6
	cmp di, 13
	jb .2
	mov al, 4
.2:
	mov [bp+timing], ax
	cmp di, 31
	je victory
	inc byte [bp+next_seq]

; show current sequence

	xor di, di
.1:
	mov al, [bp+di+memory]
	push di
	mov [bp+button], al
	call show_buttons
	mov cx, [bp+timing]
	call wait_ticks
	call speaker_off

	mov byte [bp+button], 0
	call show_buttons
	call wait_tick
	pop di
	inc di
	cmp di, [bp+next_seq]
	jne .1
	
	; empty keyboard buffer
.9:
	mov ah, 0x01
	int 0x16        ; if key pressed
	je .8
	mov ah, 0x00
	int 0x16        ; read key
	jmp .9
.8:

	; comparison of player input with sequence
	xor di, di
.4:
	mov ah, 0x00
	int 0x16
	cmp al, 0x1b
	je exit_game
	cmp al, 0x31 ; ascii '1'
	jb .4
	cmp al, 0x35    ; > ascii '4'?
	jnb .4
	push ax
	push di
	mov [bp+button], al
	call show_buttons
	mov cx, [bp+timing]
	call wait_ticks
	call speaker_off

	mov byte [bp+button], 0
	call show_buttons
	call wait_tick
	pop di
	pop ax
	cmp al, [bp+di+memory]
	jne wrong

	inc di
	cmp di, [bp+next_seq]
	jne .4
	jmp game_loop


wrong:
	mov cx, 28409
	call speaker
	mov cl, 27
	call wait_ticks
	call speaker_off
	mov cl, 27
	call wait_ticks
	jmp restart_game


victory:
	mov al, '2'
	mov cx, 14
.1:
	push cx
	push ax
	mov byte [bp+button], al
	call show_buttons
	mov cl, 2
	call wait_ticks
	mov byte [bp+button], 0
	call show_buttons
	pop ax
	inc ax
	cmp al, '5'
	jne .2
	mov al, '1'
.2:
	pop cx
	loop .1
	jmp wrong


exit_game:
	mov ax, 0x0002
	int 0x10        ; clean screen
	int 0x20        ; exit to dos


show_buttons:
	mov di, 0x0166
	mov bx, 0x312f
	mov cx, 2873
	call show_button

	mov di, 0x0192
	mov bx, 0x324f
	mov cx, 3835
	call show_button

	mov di, 0x0846
	mov bx, 0x336f
	mov cx, 4812
	call show_button

	mov di, 0x0872
	mov bx, 0x343f
	mov cx, 5746

	; fall through

show_button:
	mov al, 0x20
	cmp bh, [bp+button]
	jne .0
	call speaker
	mov al, 0xb0
.0:
	mov cx, 10
.1:
	push cx
	mov ah, bl
	mov cl, 20
	rep stosw
	add di, 160-20*2
	pop cx
	loop .1
	mov al, bh
	mov [di+20-5*160], ax
	ret

wait_tick:
	mov cl, 1

wait_ticks:
	mov ch, 0
.0:
	push cx
.1:
	mov ah, 0x00
	int 0x1a
	cmp dx, [bp+old_time]
	je .1
	mov [bp+old_time], dx
	pop cx
	loop .0
	ret


; cx = frequency
speaker:
	mov al, 0xb6
	out 0x43, al
	mov al, cl
	out 0x42, al
	mov al, ch
	out 0x42, al
	in al, 0x61
	or al, 0x03
	out 0x61, al
	ret


speaker_off:
	in al, 0x61
	and al, 0xfc
	out 0x61, al
	ret



%ifdef com_file
%else
	times 510-($-$$) db 0x4f
	db 0x55, 0xaa ; make it a bootable sector
%endif

