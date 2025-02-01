;;; core.tsp
(def (list . lst) "Create list" lst)
(def quit() '~No REPL to quit from~)

(def defmacro
  (Macro (args . body)
    "Define named macro, with argument list and body
  First element of arguments is name of macro
  Also see: def"
    (cond
      ((pair? args)
       (list 'def (car args) (list 'Macro (cdr args) . body)))
      (else
        (error 'defmacro "expected macro name and argument List, recieved "
              (typeof args))))))

;;; CXR

; TODO def car cdr with get syntax ?
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

(def (any? x)         True)
(def (void? x)        (= (typeof x) "Void"))
(def (nil? x)         (= (typeof x) "Nil"))
(def  empty?          nil?)
(def (integer? x)     (= (typeof x) "Int")) ; TODO shorten type querry funcs ?
(def (decimal? x)     (= (typeof x) "Dec"))
(def (ratio? x)       (= (typeof x) "Ratio"))
(def (string? x)      (= (typeof x) "Str"))
(def (symbol? x)      (= (typeof x) "Sym"))
(def (primitive? x)   (= (typeof x) "Prim"))
(def (specialform? x) (= (typeof x) "Form"))
(def (function? x)    (= (typeof x) "Func"))
(def (macro? x)       (= (typeof x) "Macro"))
(def (record? x)      (= (typeof x) "Rec"))
(def (pair? x)        (= (typeof x) "Pair"))
(def (atom? x)        (not (pair? x)))
(def (cons? x)        (and (pair? x) (not (pair? (cdr x)))))
(def (list? x)        (if (pair? x) (list? (cdr x)) (not x)))
(def (boolean? x)     (or (= x True) (nil? x)))
(def (true? x)        (= x True))
(def  false?          nil?)
(def (builtin? x)     (or (primitive? x) (specialform? x)))
(def (procedure? x)   (or (builtin? x) (or (function? x) (macro? x))))
(def (rational? x)    (or (integer? x) (ratio? x)))
(def (number? x)      (or (rational? x) (decimal? x)))

(def (Bool x) (if x True Nil))
; TODO handle string and sym
(def (Pair x)
  (cond
    ((rational? x)
     (cons (numerator x)
           (denominator x)))
    ((decimal? x)
     (cons (integer (truncate x))
           (- x (truncate x))))
    ((or (void? x) (nil? x) (pair? x)) x)
    (else (cons x Nil))))

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
  "Execute a if condition con is true, otherwise run b"
  (list 'cond (list con a) (list True b)))
(def else True)
(defmacro (when con . body)
  "Execute body if condition con is true"
  (list 'cond (list con (cons 'do body))))
(defmacro (unless con . body)
  "Execute body unless condition, con, is true"
  (list 'cond (list (list not con) (cons 'do body))))

(defmacro (let vars . body)
  "Execute body with new local variables in vars
  vars is a list of name and value pairs"
  (list (list* 'Func ()
               (append
                 (map
                   @(list* 'def (car it) (cdr it))
                   vars)
                 body))))

(defmacro (recur proc vars . body)
  "Do recursion within body by calling proc with values for vars
  Also see: let"
  (list 'let
        (list*
          (list proc (list* 'Func (map car vars) body))
          vars)
        (list* proc (map car vars))))

; TODO support else that is run if no values are equal
(defmacro (switch val . body)
  "Compare value to first element in each body statement, only running line where they are equal"
  (list* 'cond (map
                 @`((= ,val ,(car it)) ,(cadr it))
                 body)))

(defmacro (quasiquote expr)
  "Recursively quote the given expression
  Automatically quotes each element within the expression, but evaluates the
  element if it is labeled with the unquote macro.
  Can be shortened with the ` prefix.
  Also see: quote, unquote, unquote-splice"
  (def (check form)
    (unless (and (pair? (cdr form)) (nil? (cddr form)))
      (error (car form) "invalid form " form)))
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
  (recur f ((expr expr) (n 0))
    (cond
      ((nil? expr) Nil)
      ((atom? expr) (list 'quote expr))
      ((= (car expr) 'quasiquote)
       (check expr)
       (quasicons ''quasiquote (f (cdr expr) (+ n 1))))
      ((= (car expr) 'unquote)
       (check expr)
       (if (= n 0)
         (cadr expr)
         (quasicons ''unquote (f (cdr expr) (- n 1)))))
      ((= (car expr) 'unquote-splice)
       (check expr)
       (if (= n 0)
         (error 'unquote-splice "invalid context for " (cadr expr))
         (quasicons ''unquote-splice (f (cdr expr) (- n 1)))))
      ((and (= n 0) (and (pair? (car expr)) (= (caar expr) 'unquote-splice)))
       (check (car expr))
       (let (d: (f (cdr expr) n))
         (if d
           (list 'append (cadar expr) d)
           (cadar expr))))
      (else (quasicons (f (car expr) n) (f (cdr expr) n))))))

(defmacro (unquote expr)
  "Unquote expression so its evaluated before placed into the quasiquote
  Can be shortened with the , prefix
  Errors if called outside quasiquote
  Also see: quasiquote, unquote-splice, quote"
  (error 'unquote "called outside of quasiquote"))
(defmacro (unquote-splice expr)
  "Unquote and splice the expression into the quasiquote
  If the value evaluated is a list, embedded each element into the quasiquote
  Can be shortened with the ,@ prefix
  Errors if called outside a quasiquote of a list
  Also see: quasiquote, unquote, quote"
  (error 'unquote-splice "called outside of quasiquote"))

;;; Logic

(def False ())
(def (not x)
  (if x Nil True))
; Use a macro so arguments aren't evaluated all at once
(defmacro (and a b)
  "Return b if a is not nil, else return nil"
  (list 'if a b Nil))
(defmacro (or a b)
  "Return a if not nil, else return b"
  (list 'if a a b))
(defmacro (xor? a b)
  "Exclusive or, either a or b are true, but not if both are true"
  (list 'and (list 'or a b) (list 'not (list 'and a b))))
