org 0x7c00

start:
	push cs
	pop ds
	mov bx, string

repeat:
	mov al, [bx]       ; load char into al
	test al, al        ; check for null
	je end
	push bx

	mov ah, 0x0e       ; teletype output
	mov bx,0x000f      ; colour
	int 0x10           ; write char

	pop bx
	inc bx
	jmp repeat
	
end:
	jmp $              ; infinate loop


string:
	db "Hello, world", 0

	times 510-($-$$) db 0     ; pad to 510

	db 0x55, 0xaa             ; boot signature


