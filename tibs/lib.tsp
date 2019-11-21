(define nil ())

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
(define (void? x)      (= (type x) "void"))
(define (nil? x)       (= (type x) "nil"))
(define  empty?         nil?)
(define (integer? x)   (= (type x) "integer"))
(define (decimal? x)   (= (type x) "decimal"))
(define (ratio? x)     (= (type x) "ratio"))
(define (string? x)    (= (type x) "string"))
(define (symbol? x)    (= (type x) "symbol"))
(define (primitive? x) (= (type x) "primitive"))
(define (function? x)  (= (type x) "function"))
(define (macro? x)     (= (type x) "macro"))
(define (pair? x)      (= (type x) "pair"))
(define (cons? x)      (and (pair? x) (not (pair? (cdr x)))))
(define (list? x)      (if (pair? x) (list? (cdr x)) (not x)))
(define (boolean? x)   (or (= x t) (nil? x)))
(define (true? x)      (= x t))
(define  false?         nil?)
(define (rational? x)  (or (integer? x) (ratio? x)))
(define (number? x)    (or (rational? x) (decimal? x)))

(define assert
  (macro (condition)
         (list 'unless condition
               (list error ''assert "assertion " 'condition " failed"))))

;;; Control Flow
(define if
  (macro (con c d)
         (list 'cond (list con c) (list t d))))
(define else t)
(define when
  (macro (con . body)
         (list 'cond (list con (cons 'do body)))))
(define unless
  (macro (con . body)
         (list 'cond (list (list not con) (cons 'do body)))))
(define let
  (macro (vars . body)
         (cons (list* 'lambda (map car vars) body) (map cadr vars))))
(define recur
  (macro (fn vars . body)
         (cons (list* 'lambda (cons fn (map car vars)) body) (cons (list* 'lambda (map car vars) body) (map cadr vars)))))

;;; Logic
(define (not x)
  (if x nil t))
(define and ; Use a macro so arguments aren't evaluated all at once
  (macro (a b)
         (list 'if a b nil)))
(define nand
  (macro (a b)
         (list 'not (list 'and a b))))
(define or
  (macro (a b)
         (list 'if a a b)))
(define nor
  (macro (a b)
         (list 'not (list 'or a b))))

;;; Lists
(define first car)
(define rest cdr)
(define (list . lst) lst)
(define (list* . lst)
  (if (cdr lst)
    (cons (car lst) (apply list* (cdr lst)))
    (car lst)))

(define (do . body) (last body))
(define (do0 . body) (car body))

(define (length lst)
  (recur f ((lst lst) (x 0))
         (if lst
           (f (cdr lst) (+ x 1))
           x)))

(define (last lst)
  (if (cdr lst)
    (last (cdr lst))
    (car lst)))

(define (nth lst i)
  (when (pair? lst)
    (if (<= i 0)
      (car lst)
      (nth (cdr lst) (- i 1)))))

(define (count x lst)
  (cond ((nil? lst) 0)
        ((= x (car lst)) (+ 1 (count x (cdr lst))))
        (else (count x (cdr lst)))))

(define (apply proc args)
  (eval (map (lambda (x) ; prevent args from being evaluated twice
               (list 'quote x))
             (cons proc args))))

(define (map proc lst)
  (if lst
    (cons (proc (car lst))
          (map proc (cdr lst)))
    nil))

(define (reverse l)
  (recur f ((in l) (out nil))
         (if (pair? in)
           (f (cdr in) (cons (car in) out))
           out)))

(define (append x y)
  (if x
    (cons (car x) (append (cdr x) y))
    y))

(define (zip x y)
  (cond ((and (nil? x) (nil? y)) nil)
        ((and (pair? x) (pair? y))
         (cons (cons (car x) (car y))
               (zip (cdr x) (cdr y))))))

(define (assoc key table)
  (cond ((nil? table) nil)
        ((= key (caar table)) (car table))
        (else (assoc key (cdr table)))))

(define (memp proc lst)
  (cond ((nil? lst) nil)
        ((proc (car lst)) lst)
        (else (memp proc (cdr lst)))))

(define (member mem lst)
  (memp (lambda (x) (= mem x)) lst))

;;; Stacks
(define (push stack val)
  (cons val stack))

(define push!
  (macro (stack val)
         (list do
               (list set! stack (list push stack val))
               stack)))

(define pop cdr)

(define pop!
  (macro (stack)
         (list do0
               (list peek stack)
               (list set! stack (list pop stack)))))

(define peek car)

(define (swap stack)
  (let ((x (peek stack))
        (y (peek (pop stack))))
    (push (push (pop (pop stack)) x) y)))

(define swap!
  (macro (stack)
         (list let (list (list 'x (list pop! stack))
                         (list 'y (list pop! stack)))
               (list set! stack (list push (list push stack 'x) 'y))
               stack)))

;;; Math
(define pi (* 4 (arctan 1.)))
(define tau (* 2 pi))
(define e (exp 1.))

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

(define (csc x) (/ (sin x)))
(define (sec x) (/ (cos x)))
(define (cot x) (/ (tan x)))
(define (arccsc x) (/ (arcsin x)))
(define (arcsec x) (/ (arccos x)))
(define (arccot x) (/ (arctan x)))

(define (abs x) (if (>= x 0) x (- x)))
(define (sgn x) (cond ((= x 0) 0) ((> x 0) 1) (else -1)))
(define (max a b) (if (> a b) a b))
(define (min a b) (if (< a b) a b))

(define (positive? x) (> x 0))
(define (negative? x) (< x 0))
(define (zero? x) (= x 0))
(define (even? x) (= (mod x 2) 0))
(define (odd? x)  (= (mod x 2) 1))

(define (! n)
  (if (= n 1)
    1
    (* n (! (- n 1)))))

;;; IO
(define (print . str) (apply write (cons 'stdout str)))
(define (newline) (print "\n"))
(define (disp . str) (apply print str) (newline))
(define (disp-string . str)
  (map (lambda (s)
         (if (string? s)
           (print "\"" s "\"")
           (print s)))
       str)
  (newline))
