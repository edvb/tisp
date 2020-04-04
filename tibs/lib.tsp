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
(define (any? x)       t)
(define (void? x)      (= (type x) "void"))
(define (nil? x)       (= (type x) "nil"))
(define  empty?        nil?)
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
(define  false?        nil?)
(define (procedure? x) (or (primitive? x) (or (function? x) (macro? x))))
(define (rational? x)  (or (integer? x) (ratio? x)))
(define (number? x)    (or (rational? x) (decimal? x)))

(define (bool x) (if x t nil))
; TODO handle string and sym
(define (pair x)
  (cond
    ((rational? x)
     (cons (numerator x)
           (denominator x)))
    ((decimal? x)
     (cons (integer (truncate x))
           (- x (truncate x))))
    ((or (void? x) (nil? x) (pair? x)) x)
    (else (list x))))

(defmacro (var name eq . val)
  "define new variable with given value"
  `(if (= ',eq '=)
     ,(list* 'define name val)
     (error 'var "incorrect form")))

(defmacro (assert expr)
  `(unless ,expr
        (error 'assert "assertion " ',expr " failed")))

; TODO support any sized list n depending on size of optional val
(define (default n val)
  (cond
    ((nil? n) val)
    ((and (pair? n) (nil? (cdr n)))
     (car n))
    (else (error 'default "expected only 1 optional argument"))))

;;; Control Flow
; TODO if b = pair and car b = else use cdr b
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

(defmacro (switch val . body)
  (list* 'cond (map
                 (lambda (line)
                   `((= ,val ,(car line)) ,(cadr line)))
                 body)))

; TODO allow for improper lists
(defmacro (quasiquote x)
  (define (check x)
    (unless (and (pair? (cdr x)) (nil? (cddr x)))
      (error (car x) "invalid form " x)))
  (define (quasicons a d)
    (if (pair? d)
      (if (= (car d) 'quote)
        (if (and (pair? a) (= (car a) 'quote))
          (list 'quote (list* (cadr a) (cadr d)))
          (if (nil? (cadr d))
            (list 'list a)
            (list list* a d)))
        (if (member (car d) '(list list*))
          (list* (car d) a (cdr d))
          (list list* a d)))
      (list list* a d)))
  (recur f ((x x))
         (cond
           ((atom? x) (list 'quote x))
           ((= (car x) 'unquote)
            (check x)
            (cadr x))
           (else (quasicons (f (car x)) (f (cdr x)))))))

(defmacro (unquote x) (list error ''unquote "called outside of quasiquote"))

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
(define (nth lst n)
  (cond
    ((atom? lst)
     (error 'nth "index of list out of bounds"))
    ((<= n 0) (car lst))
    (else (nth (cdr lst) (- n 1)))))

; TODO diff name head/tail since conflicts w/ unix
; TODO support negative numers like unix tail/head to count from end backwards
(define (head lst n)
  (cond
    ((<= n 0) nil)
    ((atom? lst)
     (error 'name "index of list out of bounds"))
    (else (cons (car lst) (head (cdr lst) (- n 1))))))

(define (tail lst n)
  (cond
    ((<= n 0) lst)
    ((atom? lst)
     (error 'tail "index of list out of bounds"))
    (else (tail (cdr lst) (- n 1)))))

(define (count x lst)
  (cond ((nil? lst) 0)
        ((atom? lst) (error 'count "expected proper list"))
        ((= x (car lst)) (+ 1 (count x (cdr lst))))
        (else (count x (cdr lst)))))

; TODO many args
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

(define (filter proc lst)
  (cond
    ((not (pair? lst)) nil)
    ((proc (car lst)) (cons (car lst) (filter proc (cdr lst))))
    (else (filter proc (cdr lst)))))

(define (compose . procs)
  (cond
    ((nil? procs) (lambda x x))
    ((nil? (cdr procs)) (car procs))
    (else
      (lambda x
        ((car procs) (apply (apply compose (cdr procs)) x))))))

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
  `(set! ,stack (push ,stack ,val)))

(define pop cdr)

(defmacro (pop! stack)
  `(do0
     (peek ,stack)
     (set! ,stack (pop ,stack))))

(define peek car)

(define (swap stack)
  (let ((x (peek stack))
        (y (peek (pop stack))))
    (push (push (pop (pop stack)) x) y)))

; TODO swap! use gen sym instead of x and y ?
(defmacro (swap! stack)
  `(let ((x (pop! ,stack))
         (y (pop! ,stack)))
     (set! ,stack (push (push ,stack x) y))))

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
