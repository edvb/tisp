; infix operators and their priories, higher priories get evaluated first
(def infix-ops {
  "|>": -1 "|>>": -1 "|->": -1 "|=>": -1 ">>=": -1 "<<": -1
  ; ":=": 0 "<-": 0 "->": 0 in: 0 ":": 0
  "=>": 0
  and: 1 or: 1 nand: 1 nor: 1 to: 1
  "=": 2 "/=": 2 "<": 2 ">": 2 "<=": 2 ">=": 2
  "+": 3 "-": 3
  "*": 4 "/": 4
  "^": 5
})

(def infix-tests
  '((1 + 2 * 4): 9
    ((1 + 2) * 4): 12
    (1 to 5): '(1 2 3 4 5)
    (2^2 + 3^2): 13
                   ))
(def run-test()
  (reduce and '()))

; TODO (3 < x < 5) => (and (< 3 x) (< x 5))
; TODO (a = b = c) => (= a b c)
; (def infix-repeat-ops {
;   "=": (Func (a b c) (= a b c))
;   "<": (Func (a b c) (and (< a b) (> b c)))
; })


; TODO locally scoped, if in expr wrap func around
(defmacro (:= var val) `(do (def ,var ,val) ,var))
(def in assoc) ; TODO work for lists or assoc lists, multi purpose like awk
; (def ... Pair)
; (def in has?)
; (def <> compose) ; TODO better op
; (def => Func)
; (def ++ Str) ; * like julia?
; (def .. to)
; (def : Pair)
; (def (:: v k) v('k))
; TODO ?= prefix for ?=Bool = (x) => typeof(x) == Bool (instead of bool?) ?
; TODO unicode equivalents

(defmacro (-> val form)
  "Pipe VAL through FORM
  Insert VAL as the second item in FORM, creating a list if not a list already.
  > 3 |> (* 2) |> (- 7) ;; => 1
  > '(1 2 3) |> append([4 5 6]) ;; => (1 2 3 4 5 6)"
  (if pair?(forms)
    `(,car(forms) ,val ,@cdr(forms)) ; TODO if caar(forms) not = quote
    `(,forms ,val)))

(defmacro (|>> val forms)
  "Pipe VAL through the FORMS
  Insert VAL as the last item in the first FORM, creating a list if not a list already.
  If there are more forms, insert the first form as the last item in second FORM, etc\n
  > 4 |>> [1 2 3] |>> map(sqr) ;; => (1 4 9 16)
  > [1 2 3] |>> append([3 4 5]) ;; => (3 4 5 1 2 3)"
  (if pair?(forms)
    `(,@forms ,val)
    `(,forms ,val)))

; replaces need for apply ?
(defmacro (<| func ... args)
  "Func application at read-time"
  `(,func ,@args))

; if and append

(def has?(rec key)
  ; try? rec(key)
  assoc(key records(rec))
  )

; TODO accept many lists to append
(def (push-last x y)
  "Append y to end of list x"
  (if (pair? x)
    (cons (car x) (append (cdr x) y))
    (list y)))

(def pop-recur(ops elem popped)
  "Recursively pop the ops, returns stack of ops and list of popped elements"
  ; append to popped, not push, because order of returned popped stack should be same as was in working stack
  (if (and ops (<= infix-ops:(elem) infix-ops:(car(ops))))
    (pop-recur cdr(ops) elem append(popped list(car(ops))))
    (cons cons(elem ops) popped)))

(def apply-op(op args)
  "Replace first two entries of queue with application of op on them
  apply-op f (x y z w) ;=> ((f y x) z w)"
  (if (>= length(args) 2)
    (cons list(op snd(args) fst(args)) cddr(args))
    op
    ))

; TODO combine multi = to optimize, and fix things
(def apply-ops(ops vals)
  "Apply each operator in list in order to first two values
  Top of the stack should be the first operator to be applied
  apply-ops (* -) (1 2 3) => a-ops (-) ((* 2 1) 3) => a-ops (-) ((* 2 1) 3) => (- 3 (* 2 1))"
  (if pair?(ops)
    (apply-ops cdr(ops) apply-op(car(ops) vals))
    vals))

(def shunting-yard(queue ops exprs)
  "Shunting-yard algorithm for operators"
  (if pair?(exprs)
    (if assoc(car(exprs) records(infix-ops)) ; if expr is an operator
      (let (val: pop-recur(ops car(exprs) Nil)
            new-ops: car(val)
            popped: cdr(val))
        (shunting-yard apply-ops(popped queue) new-ops cdr(exprs)))
      ; not infix op eg Num, Sym etc
      (shunting-yard push(queue car(exprs)) ops cdr(exprs)))
    (fst apply-ops(ops queue))))

(defmacro (infix ... exprs)
  "Infix binary operations
  > 1 + 2 * 3 + 4^2 = 22
  > + = #<prim:+>
  > [1 2 3] |> length = 3
  > (1 to 5) |> filter odd? |> sum = 9"
  (shunting-yard Nil Nil exprs))
(def $= infix)
