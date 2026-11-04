; The picture and draw sequence live in Lisp, not in the native host.
; VERTEX arguments: index x y red green blue.
(define setup-triangle
  (lambda ()
    (vertex 0 320  80 255   0   0)
    (vertex 1 100 400   0 255   0)
    (vertex 2 540 400   0   0 255)))

(define draw-triangle
  (lambda ()
    (clear)
    (setup-triangle)
    (triangle 0 1 2)
    (present)))

(define show-triangle
  (lambda ()
    (glide-init)
    (draw-triangle)
    (key)
    (glide-close)))

(show-triangle)
