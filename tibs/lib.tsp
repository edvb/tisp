(define nil ())
(define (list . lst) lst)

(define defmacro
  (macro (args . body)
         (list 'define (car args) (list 'macro (cdr args) . body))))

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
(define (atom? x)      (not (pair? x)))
(define (cons? x)      (and (pair? x) (not (pair? (cdr x)))))
(define (list? x)      (if (pair? x) (list? (cdr x)) (not x)))
(define (boolean? x)   (or (= x t) (nil? x)))
(define (true? x)      (= x t))
(define  false?         nil?)
(define (rational? x)  (or (integer? x) (ratio? x)))
(define (number? x)    (or (rational? x) (decimal? x)))

(defmacro (assert condition)
  (list 'unless condition
        (list error ''assert "assertion " 'condition " failed")))

;;; Control Flow
; TODO if d = pair and car d = else use cdr d
(defmacro (if con a b)
  (list 'cond (list con a) (list t b)))
(define else t)
(defmacro (when con . body)
  (list 'cond (list con (cons 'do body))))
(defmacro (unless con . body)
  (list 'cond (list (list not con) (cons 'do body))))
(defmacro (let vars . body)
  (list (list* 'lambda ()
               (append
                 (map
                   (lambda (x)
                     (list* 'define (car x) (cdr x)))
                   vars)
                 body))))
(defmacro (recur proc vars . body)
  (list 'let
        (list*
          (list proc (list* 'lambda (map car vars) body))
          vars)
        (list* proc (map car vars))))

;;; Logic
(define true t)
(define false ())
(define (not x)
  (if x nil t))
; TODO logic func many arguments
; Use a macro so arguments aren't evaluated all at once
(defmacro (and a b)
  (list 'if a b nil))
(defmacro (nand a b)
  (list 'not (list 'and a b)))
(defmacro (or a b)
  (list 'if a a b))
(defmacro (nor a b)
  (list 'not (list 'or a b)))

;;; Lists
(define (list* . lst)
  (if (cdr lst)
    (cons (car lst) (apply list* (cdr lst)))
    (car lst)))

(define (do  . body) (last body))
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

; TODO make nth generic for list str vec, made up of list-ref vec-ref str-ref
(define (nth lst i)
  (if (pair? lst)
    (if (<= i 0)
      (car lst)
      (nth (cdr lst) (- i 1)))
    (error 'nth "index of list out bounds")))

(define (count x lst)
  (cond ((nil? lst) 0)
        ((atom? lst) (error 'count "expected proper list"))
        ((= x (car lst)) (+ 1 (count x (cdr lst))))
        (else (count x (cdr lst)))))

(define (apply proc args)
  (eval (map (lambda (x) ; prevent args from being evaluated twice
               (list 'quote x))
             (cons proc args))))

; TODO many lsts for proc w/ multi arguments
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

; TODO accept many lists to append
(define (append x y)
  (cond
    ((pair? x) (cons (car x) (append (cdr x) y)))
    ((nil? x) y)
    (else (error 'append "expected proper list"))))

(define (zip x y)
  (cond ((and (nil? x) (nil? y)) nil)
        ((or (nil? x) (nil? y)) (error 'zip "given lists of unequal length"))
        ((and (pair? x) (pair? y))
         (cons (cons (car x) (car y))
               (zip (cdr x) (cdr y))))))

; TODO assoc optional equal? arg
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

; define English list element accessors
(define rest cdr) ; TODO first and rest are generics for list, vec, str types
(define first car)
(let (((def name count)
       (list 'define (list name 'x) (list 'nth 'x count))))
  (recur f ((n 1)
            (lst '(second third forth fifth sixth seventh eighth ninth tenth)))
         (when lst
           (eval (def (car lst) n))
           (f (+ n 1) (cdr lst)))))

;;; Stacks
(define (push stack val)
  (cons val stack))

(defmacro (push! stack val)
  (list do
        (list set! stack (list push stack val))
        stack))

(define pop cdr)

(defmacro (pop! stack)
  (list do0
        (list peek stack)
        (list set! stack (list pop stack))))

(define peek car)

(define (swap stack)
  (let ((x (peek stack))
        (y (peek (pop stack))))
    (push (push (pop (pop stack)) x) y)))

; TODO swap! use gen sym instead of x and y ?
(defmacro (swap! stack)
  (list let (list (list 'x (list pop! stack))
                  (list 'y (list pop! stack)))
        (list set! stack (list push (list push stack 'x) 'y))
        stack))

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

; define inverse trig functions
(let (((def inv func)
       (eval (list 'define (list inv 'x) (list '/ (list func 'x)))))
      ((def* inv func)
       (def inv func)
       (def (symbol inv 'h) (symbol func 'h)))
      ((def** inv func)
       (def* inv func)
       (def* (symbol 'arc inv) (symbol 'arc func))))
  (recur f ((lst '((csc sin) (sec cos) (cot tan))))
         (when lst
           (apply def** (car lst))
           (f (cdr lst)))))

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

(define (dot x y)
  (if x
    (+ (* (car x) (car y))
       (dot (cdr x) (cdr y)))
    0))

(define (! n)
  (if (= n 1)
    1
    (* n (! (- n 1)))))

;;; IO
(define (print . str) (apply write (list* 'stdout nil str)))
(define (newline . file)
  (if (or (nil? file) (nil? (cdr file)))
    (write (car (or file '(stdout))) file "\n")
    (error 'newline "only zero or one file can be given")))
(define (disp . str) (apply print str) (newline))
; TODO rename displayln, also quote syms and lists
(define (disp-string . str)
  (map (lambda (s)
         (if (string? s)
           (print "\"" s "\"")
           (print s)))
       str)
  (newline))
