\ This file defines the picture. The host contains no triangle coordinates.
\ vertex! takes ( x y red green blue index -- ).
: red   255 0 0 ;
: green 0 255 0 ;
: blue  0 0 255 ;

: setup-triangle
    320 80  red   0 vertex!
    100 400 green 1 vertex!
    540 400 blue  2 vertex!
;

: draw-triangle
    clear
    setup-triangle
    0 1 2 triangle
    present
;

: show-triangle
    glide-init
    draw-triangle
    key drop
    glide-close
;

show-triangle
