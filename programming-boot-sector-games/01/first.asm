
   ; The incredible Hello, World program

   org 0x0100        ; Start pont of program for a COM file
start:
   mov bx, string    ; Load register BX with address of 'string'
repeat:
   mov  al, [bx]     ; Load a byte in AL from adress pointed by BX
   test al, al       ; Test AL for zero
   je   end          ; Jump if equal to 'end' label
   push bx           ; Save BX to the stack
   mov  ah, 0x0e     ; load ah with the code for terminal output
   mov  bx, 0x000f   ; 
   int  0x10         ; Call the BIOS for displaying one letter
   pop  bx           ; restore bx
   inc  bx           ; move the pointer to the next letter
   jmp  repeat

end:
   int 0x20          ; end to command-line

string:
   db "Hello, world", 0

