\ Core semantics, compiled literals, nested definitions and native memory access.
1 2 + 3 = assert
-12 3 * -36 = assert
$ff 255 = assert
-2147483648 -2147483648 = assert
0 0= -1 = assert
17 0= 0 = assert
15 7 nand -8 = assert
12 dup + 24 = assert
1 2 swap 1 = assert 2 = assert
1 2 over 1 = assert drop drop
10 3 - 7 = assert
depth 0 = assert
42 sp@ @ 42 = assert drop
here @ dup 123 swap ! @ 123 = assert
: twice dup + ;
: six 3 twice ;
six 6 = assert
: early 19 exit 99 ;
early 19 = assert
: nested six twice ;
nested 12 = assert
\ Case folding and empty definitions.
: NOTHING ; NOTHING
STATE @ 0 = assert
LATEST @ 0= 0= assert
\ Verify return-stack access from a compiled word.
: return-address rp@ @ 0= 0= assert ;
return-address
