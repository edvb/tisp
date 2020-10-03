(def (list . lst) lst)

(def defmacro
  (macro (args . body)
         (list 'def (car args) (list 'macro (cdr args) . body))))

;;; CXR

(def (caar x) (car (car x)))
(def (cadr x) (car (cdr x)))
(def (cdar x) (cdr (car x)))
(def (cddr x) (cdr (cdr x)))
(def (caaar x) (car (car (car x))))
(def (caadr x) (car (car (cdr x))))
(def (cadar x) (car (cdr (car x))))
(def (caddr x) (car (cdr (cdr x))))
(def (cdaar x) (cdr (car (car x))))
(def (cdadr x) (cdr (car (cdr x))))
(def (cddar x) (cdr (cdr (car x))))
(def (cdddr x) (cdr (cdr (cdr x))))
(def (caaaar x) (car (car (car (car x)))))
(def (caaadr x) (car (car (car (cdr x)))))
(def (caadar x) (car (car (cdr (car x)))))
(def (caaddr x) (car (car (cdr (cdr x)))))
(def (cadaar x) (car (cdr (car (car x)))))
(def (cadadr x) (car (cdr (car (cdr x)))))
(def (caddar x) (car (cdr (cdr (car x)))))
(def (cadddr x) (car (cdr (cdr (cdr x)))))
(def (cdaaar x) (cdr (car (car (car x)))))
(def (cdaadr x) (cdr (car (car (cdr x)))))
(def (cdadar x) (cdr (car (cdr (car x)))))
(def (cdaddr x) (cdr (car (cdr (cdr x)))))
(def (cddaar x) (cdr (cdr (car (car x)))))
(def (cddadr x) (cdr (cdr (car (cdr x)))))
(def (cdddar x) (cdr (cdr (cdr (car x)))))
(def (cddddr x) (cdr (cdr (cdr (cdr x)))))

;;; Types

(def (any? x)       True)
(def (void? x)      (= (typeof x) "Void"))
(def (nil? x)       (= (typeof x) "Nil"))
(def  empty?        nil?)
(def (integer? x)   (= (typeof x) "Int"))
(def (decimal? x)   (= (typeof x) "Dec"))
(def (ratio? x)     (= (typeof x) "Ratio"))
(def (string? x)    (= (typeof x) "Str"))
(def (symbol? x)    (= (typeof x) "Sym"))
(def (primitive? x) (= (typeof x) "Prim"))
(def (function? x)  (= (typeof x) "Func"))
(def (macro? x)     (= (typeof x) "Macro"))
(def (pair? x)      (= (typeof x) "Pair"))
(def (atom? x)      (not (pair? x)))
(def (cons? x)      (and (pair? x) (not (pair? (cdr x)))))
(def (list? x)      (if (pair? x) (list? (cdr x)) (not x)))
(def (boolean? x)   (or (= x True) (nil? x)))
(def (true? x)      (= x True))
(def  false?        nil?)
(def (procedure? x) (or (primitive? x) (or (function? x) (macro? x))))
(def (rational? x)  (or (integer? x) (ratio? x)))
(def (number? x)    (or (rational? x) (decimal? x)))

(def (bool x) (if x True Nil))
; TODO handle string and sym
(def (pair x)
  (cond
    ((rational? x)
     (cons (numerator x)
           (denominator x)))
    ((decimal? x)
     (cons (integer (truncate x))
           (- x (truncate x))))
    ((or (void? x) (nil? x) (pair? x)) x)
    (else (list x))))

(defmacro (assert expr)
  `(unless ,expr
        (error 'assert "assertion " ',expr " failed")))

; TODO support any sized list n depending on size of optional val
(def (default n val)
  (cond
    ((nil? n) val)
    ((and (pair? n) (nil? (cdr n)))
     (car n))
    (else (error 'default "expected only 1 optional argument"))))

;;; Control Flow

; TODO if b = pair and car b = else use cdr b
(defmacro (if con a b)
  (list 'cond (list con a) (list True b)))
(def else True)
(defmacro (when con . body)
  (list 'cond (list con (cons 'do body))))
(defmacro (unless con . body)
  (list 'cond (list (list not con) (cons 'do body))))

(defmacro (let vars . body)
  (list (list* 'lambda ()
               (append
                 (map
                   (lambda (x)
                     (list* 'def (car x) (cdr x)))
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
  (def (check x)
    (unless (and (pair? (cdr x)) (nil? (cddr x)))
      (error (car x) "invalid form " x)))
  (def (quasicons a d)
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

(def False ())
(def (not x)
  (if x Nil True))
; TODO logic func many arguments
; Use a macro so arguments aren't evaluated all at once
(defmacro (and a b)
  (list 'if a b Nil))
(defmacro (nand a b)
  (list 'not (list 'and a b)))
(defmacro (or a b)
  (list 'if a a b))
(defmacro (nor a b)
  (list 'not (list 'or a b)))

;;; Lists

(def (list* . lst)
  (if (cdr lst)
    (cons (car lst) (apply list* (cdr lst)))
    (car lst)))

(def (do  . body) (last body))
(def (do0 . body) (car body))

(def (length lst)
  (recur f ((lst lst) (x 0))
         (if lst
           (f (cdr lst) (+ x 1))
           x)))

(def (last lst)
; recur loop ((lst lst) (n (if n (car n) 0)))
  (if (cdr lst)
    (last (cdr lst))
    (car lst)))

; TODO make nth generic for list str vec, made up of list-ref vec-ref str-ref
(def (nth lst n)
  (cond
    ((atom? lst)
     (error 'nth "index of list out of bounds"))
    ((<= n 0) (car lst))
    (else (nth (cdr lst) (- n 1)))))

; TODO diff name head/tail since conflicts w/ unix
; TODO support negative numers like unix tail/head to count from end backwards
(def (head lst n)
  (cond
    ((<= n 0) Nil)
    ((atom? lst)
     (error 'name "index of list out of bounds"))
    (else (cons (car lst) (head (cdr lst) (- n 1))))))

(def (tail lst n)
  (cond
    ((<= n 0) lst)
    ((atom? lst)
     (error 'tail "index of list out of bounds"))
    (else (tail (cdr lst) (- n 1)))))

(def (count x lst)
  (cond ((nil? lst) 0)
        ((atom? lst) (error 'count "expected proper list"))
        ((= x (car lst)) (+ 1 (count x (cdr lst))))
        (else (count x (cdr lst)))))

; TODO many args
(def (apply proc args)
  (eval (map (lambda (x) ; prevent args from being evaluated twice
               (list 'quote x))
             (cons proc args))))

; TODO many lsts for proc w/ multi arguments
(def (map proc lst)
  (if lst
    (cons (proc (car lst))
          (map proc (cdr lst)))
    Nil))

(def (filter proc lst)
  (cond
    ((not (pair? lst)) Nil)
    ((proc (car lst)) (cons (car lst) (filter proc (cdr lst))))
    (else (filter proc (cdr lst)))))

(def (compose . procs)
  (cond
    ((nil? procs) (lambda x x))
    ((nil? (cdr procs)) (car procs))
    (else
      (lambda x
        ((car procs) (apply (apply compose (cdr procs)) x))))))

(def (reverse l)
  (recur f ((in l) (out Nil))
         (if (pair? in)
           (f (cdr in) (cons (car in) out))
           out)))

; TODO accept many lists to append
(def (append x y)
  (cond
    ((pair? x) (cons (car x) (append (cdr x) y)))
    ((nil? x) y)
    (else (error 'append "expected proper list"))))

(def (zip x y)
  (cond ((and (nil? x) (nil? y)) Nil)
        ((or (nil? x) (nil? y)) (error 'zip "given lists of unequal length"))
        ((and (pair? x) (pair? y))
         (cons (cons (car x) (car y))
               (zip (cdr x) (cdr y))))))

; TODO assoc optional equal? arg
(def (assoc key table)
  (cond ((nil? table) Nil)
        ((= key (caar table)) (car table))
        (else (assoc key (cdr table)))))

(def (memp proc lst)
  (cond ((nil? lst) Nil)
        ((proc (car lst)) lst)
        (else (memp proc (cdr lst)))))

(def (member mem lst)
  (memp (lambda (x) (= mem x)) lst))

; def English list element accessors
(def rest cdr) ; TODO first and rest are generics for list, vec, str types
(def first car)
(let (((defi name count)
       (list 'def (list name 'x) (list 'nth 'x count))))
  (recur f ((n 1)
            (lst '(second third forth fifth sixth seventh eighth ninth tenth)))
         (when lst
           (eval (defi (car lst) n))
           (f (+ n 1) (cdr lst)))))

;;; Stacks

(def (push stack val)
  (cons val stack))

(defmacro (push! stack val)
  `(set! ,stack (push ,stack ,val)))

(def pop cdr)

(defmacro (pop! stack)
  `(do0
     (peek ,stack)
     (set! ,stack (pop ,stack))))

(def peek car)

(def (swap stack)
  (let ((x (peek stack))
        (y (peek (pop stack))))
    (push (push (pop (pop stack)) x) y)))

; TODO swap! use gen sym instead of x and y ?
(defmacro (swap! stack)
  `(let ((x (pop! ,stack))
         (y (pop! ,stack)))
     (set! ,stack (push (push ,stack x) y))))
