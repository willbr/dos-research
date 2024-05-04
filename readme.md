```asm
inc      ; increment by one
int      ; interruption (call service vector)
je       ; jump if equal
jz       ; jump if zero
jmp      ; jump always
pop      ; restore data from stack
push     ; save data into stack
test     ; test register mask or zero
```


```asm
xor
or
and
not
neg
shl
shr
sar
```

```asm
add
and
call
cmp
dec
div
idiv
imul
in
mul
neg
nop
not
or
rcl
rcr
ret
rol
ror
sar
shl
sal
shr
sub
xor
```



# Addressing modes


```
[bx]
[mem]
[si]
[di]
[bx+si]
[bx+di]
[bp+si]
[bp+di]

[bx+d8]
[bp+d8]
[si+d8]
[di+d8]
[bx+si+d8]
[bx+di+d8]
[bp+si+d8]
[bp+di+d8]

[bx+d16]
[bp+d16]
[si+d16]
[di+d16]
[bx+si+d16]
[bx+di+d16]
[bp+si+d16]
[bp+di+d16]
```



# Jump instructions

```asm

je  d8
jz  d8

jne d8
jnz d8

jc  d8
jb  d8

jnc d8
jae d8

jbe d8
ja  d8

jl  d8
jg  d8
jle d8
jge d8

js  d8
jns d8

jpe d8
jpo d8

jo  d8
jno d8

jcxz d8

loop d8

; loop repeats until cx is 0
; for (cx = 80 ;cx > 0; cx -= 1) {
;     scroll_scenery();
; }

   mov cx, 80
fb1:
   push cx
   call scroll_scenery
   pop cx
   loop fb1


```

# New

```asm
cbw
jcxz
loop

lds
les
```


# Strings

```asm
std       ; set direction flag for decrementing string operations
cld       ; clear direction flag
stosb     ; store byte ; *(es+di) = al; di += 1
stosw     ; store word ; *(es+di) = ax; di += 2

cs lodsb  ; ax = *(memory+si); si += 1
cs lodsw  ; ax = *(memory+si); si += 2

pushf     ; push flags
popf      ; retore flags

```

