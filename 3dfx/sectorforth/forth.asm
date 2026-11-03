; 32-bit DOS port of Cesar Blum's sectorforth (MIT, 2020).
; See upstream/LICENSE and README.md. Four-byte cells and direct threading;
; native ESP is reserved for C calls, EBX is data SP, EBP is return SP.
bits 32
cpu 586
segment _TEXT public align=16 class=CODE use32
global _sf_eval, _sf_reset, _sf_depth, _sf_compiling
extern _host_number, _host_emit, _host_key, _host_dot
extern _gfx_init, _gfx_close, _gfx_clear, _gfx_vertex, _gfx_triangle, _gfx_swap

%define IMMEDIATE 128
%define HIDDEN 64
%define LENMASK 31
%define NEXT jmp next
%define link 0
%macro word 2-3 0
header_%2:
    dd link
%define link header_%2
%strlen %%len %1
    db %%len | %3
    db %1
%2:
%endmacro
%macro popdata 1
    cmp ebx,data_end
    jae underflow
    mov %1,[ebx]
    add ebx,4
%endmacro
%macro pushdata 1
    cmp ebx,data_stack
    jbe overflow
    sub ebx,4
    mov [ebx],%1
%endmacro
%macro room 1
    cmp dword [here],arena_end-%1
    ja dictionary_full
%endmacro

next:
    lodsd
    jmp eax

word '@',FETCH
    popdata eax
    mov eax,[eax]
    pushdata eax
    NEXT
word '!',STORE
    popdata edx
    popdata eax
    mov [edx],eax
    NEXT
word 'sp@',SPFETCH
    mov eax,ebx
    pushdata eax
    NEXT
word 'rp@',RPFETCH
    pushdata ebp
    NEXT
word '0=',ZEROEQUALS
    popdata eax
    test eax,eax
    setz al
    movzx eax,al
    neg eax
    pushdata eax
    NEXT
word '+',PLUS
    popdata edx
    popdata eax
    add eax,edx
    pushdata eax
    NEXT
word 'nand',NAND
    popdata edx
    popdata eax
    and eax,edx
    not eax
    pushdata eax
    NEXT
word 'exit',EXIT
    cmp ebp,return_end
    jae underflow
    mov esi,[ebp]
    add ebp,4
    NEXT
word 'tib',TIBVAR
    mov eax,[source]
    pushdata eax
    NEXT
word 'state',STATEVAR
    pushdata dword state
    NEXT
word '>in',TOINVAR
    pushdata dword toin
    NEXT
word 'here',HEREVAR
    pushdata dword here
    NEXT
word 'latest',LATESTVAR
    pushdata dword latest
    NEXT
word 'key',KEY
    call _host_key
    pushdata eax
    NEXT
word 'emit',EMIT
    popdata eax
    push eax
    call _host_emit
    add esp,4
    NEXT

word ':',COLON
    cmp dword [state],0
    jne compile_error
    call token
    test ecx,ecx
    jz compile_error
    cmp ecx,LENMASK
    ja compile_error
    room 41
    mov edx,[here]
    mov [pending_start],edx
    mov eax,[latest]
    mov [pending_link],eax
    mov [edx],eax
    mov [latest],edx
    mov eax,ecx
    or al,HIDDEN
    mov [edx+4],al
    push esi
    mov esi,edi
    lea edi,[edx+5]
    rep movsb
    mov byte [edi],0xe9
    mov eax,DOCOL
    lea edx,[edi+5]
    sub eax,edx
    mov [edi+1],eax
    mov [here],edx
    mov dword [state],1
    pop esi
    NEXT
DOCOL:
    cmp ebp,return_stack
    jbe overflow
    sub ebp,4
    mov [ebp],esi
    lea esi,[eax+5]
    NEXT
word ';',SEMICOLON,IMMEDIATE
    cmp dword [state],1
    jne compile_error
    room 4
    mov edi,[here]
    mov dword [edi],EXIT
    add edi,4
    mov [here],edi
    mov edx,[latest]
    and byte [edx+4],~HIDDEN
    mov dword [state],0
    NEXT

; Convenience words beyond the original minimal kernel.
word 'dup',DUP
    popdata eax
    pushdata eax
    pushdata eax
    NEXT
word 'drop',DROP
    popdata eax
    NEXT
word 'swap',SWAP
    popdata eax
    popdata edx
    pushdata eax
    pushdata edx
    NEXT
word 'over',OVER
    popdata eax
    popdata edx
    pushdata edx
    pushdata eax
    pushdata edx
    NEXT
word '-',MINUS
    popdata edx
    popdata eax
    sub eax,edx
    pushdata eax
    NEXT
word '*',MULTIPLY
    popdata edx
    popdata eax
    imul eax,edx
    pushdata eax
    NEXT
word '=',EQUALS
    popdata edx
    popdata eax
    cmp eax,edx
    sete al
    movzx eax,al
    neg eax
    pushdata eax
    NEXT
word 'depth',DEPTH
    mov eax,data_end
    sub eax,ebx
    shr eax,2
    pushdata eax
    NEXT
word '.',DOT
    popdata eax
    push eax
    call _host_dot
    add esp,4
    NEXT
word 'assert',ASSERT
    popdata eax
    test eax,eax
    jz assertion_failed
    NEXT

; Graphics ABI: C wrappers use __cdecl; preserve EBX/ESI/EDI/EBP.
word 'glide-init',GLIDEINIT
    call _gfx_init
    test eax,eax
    jz graphics_error
    NEXT
word 'glide-close',GLIDECLOSE
    call _gfx_close
    NEXT
word 'clear',CLEAR
    call _gfx_clear
    test eax,eax
    jz graphics_error
    NEXT
word 'vertex!',VERTEX
    ; ( x y r g b index -- ); push native args in reverse order.
    mov ecx,6
.args:
    popdata eax
    push eax
    loop .args
    call _gfx_vertex
    add esp,24
    test eax,eax
    jz graphics_error
    NEXT
word 'triangle',TRIANGLE
    ; ( a b c -- )
    popdata eax
    push eax
    popdata eax
    push eax
    popdata eax
    push eax
    call _gfx_triangle
    add esp,12
    test eax,eax
    jz graphics_error
    NEXT
word 'present',PRESENT
    call _gfx_swap
    test eax,eax
    jz graphics_error
    NEXT

LIT:
    lodsd
    pushdata eax
    NEXT

; C entry: int __cdecl sf_eval(const char *source).
; Dictionary and data stack persist across evaluations, as does compilation.
_sf_eval:
    push ebp
    push esi
    push edi
    push ebx
    mov [native_sp],esp
    mov eax,[esp+20]
    mov [source],eax
    mov dword [toin],0
    mov ebx,[saved_data]
    mov ebp,return_end
interpreter:
    call token
    test ecx,ecx
    jz success
    mov [token_ptr],edi
    mov [token_len],ecx
    mov edx,[latest]
.search:
    test edx,edx
    jz .number
    mov al,[edx+4]
    test al,HIDDEN
    jnz .previous
    and eax,LENMASK
    cmp eax,ecx
    jne .previous
    lea esi,[edx+5]
    push edi
    push ecx
    repe cmpsb
    pop ecx
    pop edi
    je .found
.previous:
    mov edx,[edx]
    jmp .search
.found:
    mov eax,esi
    mov esi,interpreter_thread
    test byte [edx+4],IMMEDIATE
    jnz .execute
    cmp dword [state],0
    jne .compile
.execute:
    jmp eax
.compile:
    room 4
    mov edi,[here]
    stosd
    mov [here],edi
    jmp interpreter
.number:
    push dword number_value
    push ecx
    push edi
    call _host_number
    add esp,12
    test eax,eax
    jz unknown_word
    mov eax,[number_value]
    cmp dword [state],0
    jne .literal
    pushdata eax
    jmp interpreter
.literal:
    room 8
    mov edi,[here]
    mov dword [edi],LIT
    mov [edi+4],eax
    add edi,8
    mov [here],edi
    jmp interpreter

; Tokenizer: whitespace, backslash-to-EOL comments and ( comments ).
; Source is writable; fold ASCII names to lower case in place.
token:
    mov edi,[source]
    add edi,[toin]
.skip:
    mov al,[edi]
    test al,al
    jz .empty
    cmp al,32
    ja .comment
    inc edi
    jmp .skip
.comment:
    cmp al,92
    jne .paren
.line:
    inc edi
    mov al,[edi]
    test al,al
    jz .empty
    cmp al,10
    jne .line
    jmp .skip
.paren:
    cmp al,'('
    jne .start
.paren_loop:
    inc edi
    mov al,[edi]
    test al,al
    jz compile_error
    cmp al,')'
    jne .paren_loop
    inc edi
    jmp .skip
.start:
    mov edx,edi
    xor ecx,ecx
.scan:
    mov al,[edx]
    cmp al,32
    jbe .end
    cmp al,'A'
    jb .unchanged
    cmp al,'Z'
    ja .unchanged
    add byte [edx],32
.unchanged:
    inc edx
    inc ecx
    jmp .scan
.end:
    sub edx,[source]
    mov [toin],edx
    ret
.empty:
    xor ecx,ecx
    ret

success:
    mov [saved_data],ebx
    xor eax,eax
    jmp finish
unknown_word: mov eax,1
    jmp abort_eval
underflow: mov eax,2
    jmp abort_eval
overflow: mov eax,3
    jmp abort_eval
compile_error: mov eax,4
    jmp abort_eval
dictionary_full: mov eax,5
    jmp abort_eval
assertion_failed: mov eax,6
    jmp abort_eval
graphics_error: mov eax,7
abort_eval:
    mov dword [saved_data],data_end
    cmp dword [state],0
    je .reset_state
    mov edx,[pending_start]
    mov [here],edx
    mov edx,[pending_link]
    mov [latest],edx
.reset_state:
    mov dword [state],0
finish:
    mov esp,[native_sp]
    pop ebx
    pop edi
    pop esi
    pop ebp
    ret
_sf_reset:
    mov dword [saved_data],data_end
    mov dword [here],arena
    mov dword [latest],link
    mov dword [state],0
    ret
_sf_depth:
    mov eax,data_end
    sub eax,[saved_data]
    shr eax,2
    ret
_sf_compiling:
    mov eax,[state]
    ret

segment _DATA public align=4 class=DATA use32
interpreter_thread: dd interpreter
here: dd arena
latest: dd link
state: dd 0
toin: dd 0
saved_data: dd data_end
source: dd 0
native_sp: dd 0
token_ptr: dd 0
token_len: dd 0
number_value: dd 0
pending_start: dd 0
pending_link: dd 0
segment _BSS public align=16 class=BSS use32
data_stack: resd 1024
data_end:
return_stack: resd 1024
return_end:
arena: resb 262144
arena_end:
group DGROUP _DATA _BSS
