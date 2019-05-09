;;; CXR
(define (caar x) (car (car x)))
(define (cadr x) (car (cdr x)))
(define (cdar x) (cdr (car x)))
(define (cddr x) (cdr (cdr x)))
(define (caaar x) (car (car (car x))))
(define (caadr x) (car (car (cdr x))))
(define (cadar x) (car (cdr (car x))))
(define (caddr x) (car (cdr (cdr x))))
(define (cdaar x) (cdr (car (car x))))
(define (cdadr x) (cdr (car (cdr x))))
(define (cddar x) (cdr (cdr (car x))))
(define (cdddr x) (cdr (cdr (cdr x))))
(define (caaaar x) (car (car (car (car x)))))
(define (caaadr x) (car (car (car (cdr x)))))
(define (caadar x) (car (car (cdr (car x)))))
(define (caaddr x) (car (car (cdr (cdr x)))))
(define (cadaar x) (car (cdr (car (car x)))))
(define (cadadr x) (car (cdr (car (cdr x)))))
(define (caddar x) (car (cdr (cdr (car x)))))
(define (cadddr x) (car (cdr (cdr (cdr x)))))
(define (cdaaar x) (cdr (car (car (car x)))))
(define (cdaadr x) (cdr (car (car (cdr x)))))
(define (cdadar x) (cdr (car (cdr (car x)))))
(define (cdaddr x) (cdr (car (cdr (cdr x)))))
(define (cddaar x) (cdr (cdr (car (car x)))))
(define (cddadr x) (cdr (cdr (car (cdr x)))))
(define (cdddar x) (cdr (cdr (cdr (car x)))))
(define (cddddr x) (cdr (cdr (cdr (cdr x)))))

;;; Types
(define (void? x)
  (cond ((= (type x) "void") t)
        (t ())))
(define (nil? x)
  (cond ((= (type x) "nil") t)
        (t ())))
(define (integer? x)
  (cond ((= (type x) "integer") t)
        (t ())))
(define (decimal? x)
  (cond ((= (type x) "decimal") t)
        (t ())))
(define (ratio? x)
  (cond ((= (type x) "ratio") t)
        (t ())))
(define (string? x)
  (cond ((= (type x) "string") t)
        (t ())))
(define (symbol? x)
  (cond ((= (type x) "symbol") t)
        (t ())))
(define (primitive? x)
  (cond ((= (type x) "primitive") t)
        (t ())))
(define (function? x)
  (cond ((= (type x) "function") t)
        (t ())))
(define (pair? x)
  (cond ((= (type x) "pair") t)
        (t ())))

;;; Control Flow
(define if
  (macro (con a b)
         (list 'cond (list con a) (list t b))))
(define else t)
(define when
  (macro (con . body)
         (list 'cond (list con (cons 'do body)) (list t (void)))))
(define unless
  (macro (con . body)
         (list 'cond (list con (void)) (list t (cons 'do body)))))
(define let
  (macro (vars body)
         (cons (list 'lambda (map car vars) body) (map cadr vars))))

;;; Logic
(define (not x)
  (if x () t))
(define and ; Use a macro so arguments aren't evaluated all at once
  (macro (a b)
         (list 'if a (list 'if b t ()) ())))
(define nand
  (macro (a b)
         (list 'not (list 'and a b))))
(define (or a b) ; De Morgan's theorem
  (nand (not a) (not b)))
(define (nor a b)
  (and (not a) (not b)))

;;; Lists
(define first car)
(define rest cdr)
(define (list . lst) lst)
(define (last lst)
  (cond
    ((cdr lst) (last (cdr lst)))
    (else (car lst))))
(define (apply fn args)
  (eval (cons fn args)))
(define (map fn lst)
  (cond
    (lst (cons (fn (car lst))
               (map fn (cdr lst))))
    (else ())))


;;; Math
(define pi (* 4 (arctan 1.)))
(define tau (* 2 pi))
(define e (exp 1.))

(define (sqr x) (* x x))
(define (root b p) (pow b (/ 1 p)))
(define (sqrt x) (root x 2))
(define (cbrt x) (root x 3))
(define (logb b x) (/ (log x) (log b)))
(define (log10 x) (logb 10. x))

(define (abs x) (if (>= x 0) x (- x)))
(define (sgn x) (if (>= x 0) 1 -1))
(define (max a b) (if (> a b) a b))
(define (min a b) (if (< a b) a b))

(define (positive? x) (if (> x 0) t ()))
(define (negative? x) (if (< x 0) t ()))

(define (! n)
  (if (= n 1)
    1
    (* n (! (- n 1)))))

;;; Printing
(define (newline) (print "\n"))
(define (disp . x) (apply print x) (newline))
(define (disp-string str)
  (if (string? str)
    (print "\"" str "\"")
    (print str))
  (newline))
