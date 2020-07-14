;;; Constants
(define pi (* 4 (arctan 1.)))
(define tau (* 2 pi))
(define e (exp 1.))

;;; Functions
(define (inc x) (+ x 1))
(define (dec x) (- x 1))
(define (truncate x) (* (floor (abs x)) (sgn x)))
(define (sqr x) (* x x))
(define (cube x) (* x (* x x)))
(define (root b p) (^ b (/ p)))
(define (sqrt x) (root x 2))
(define (cbrt x) (root x 3))
(define (logb b x) (/ (log x) (log b)))
(define (log10 x) (logb 10. x))

(defmacro (++ x . n)
  `(set! ,x (+ ,x ,(default n 1))))
(defmacro (-- x . n)
  `(set! ,x (- ,x ,(default n 1))))

; inverse trig functions
(define (csc x)     (/ (sin x)))
(define (arccsc x)  (/ (arcsin x)))
(define (csch x)    (/ (sinh x)))
(define (arccsch x) (/ (arcsinh x)))
(define (sec x)     (/ (cos x)))
(define (arcsec x)  (/ (arccos x)))
(define (sech x)    (/ (cosh x)))
(define (arcsech x) (/ (arccosh x)))
(define (cot x)     (/ (tan x)))
(define (arccot x)  (/ (arctan x)))
(define (coth x)    (/ (tanh x)))
(define (arccoth x) (/ (arctanh x)))

(define (abs x) (if (>= x 0) x (- x)))
(define (sgn x) (if (= x 0) x (/ (abs x) x)))
; TODO many args
(define (max a b) (if (> a b) a b))
(define (min a b) (if (< a b) a b))

(define (positive? x) (> x 0))
(define (negative? x) (< x 0))
(define (zero? x) (= x 0))
(define (even? x) (= (mod x 2) 0))
(define (odd? x)  (= (mod x 2) 1))

(define (dot v w)
  (if v
    (+ (* (car v) (car w))
       (dot (cdr v) (cdr w)))
    0))
(define (norm v) (sqrt (dot v v)))

(define (! n)
  (if (= n 1)
    1
    (* n (! (- n 1)))))

