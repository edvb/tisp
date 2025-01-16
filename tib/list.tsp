;;; Lists

; TODO rename to remove *
(def (list* . lst)
  "Create improper list, last element is not Nil"
  (if (cdr lst)
    (cons (car lst) (apply list* (cdr lst)))
    (car lst)))

(def (do0 . body)
  "Evaluate each expression in body, returning first
  Also see: do"
  (car body))

(def (length lst)
  "Number of elements in given list"
  (recur f ((lst lst) (x 0))
    (if (pair? lst)
      (f (cdr lst) (+ x 1))
      x)))

(def (last lst)
  "Last element of list"
; recur loop ((lst lst) (n (if n (car n) 0)))
  (if (cdr lst)
    (last (cdr lst))
    (car lst)))

; TODO make nth generic for list str vec, made up of list-ref vec-ref str-ref
(def (nth lst n)
  "Element number n of list, starting from 0
  If negative get number from end of list"
  (cond
    ((atom? lst)
     (error 'nth "index of list out of bounds"))
    ((< n 0) (nth lst (+ (length lst) n)))
    ((= n 0) (car lst))
    (else (nth (cdr lst) (- n 1)))))

; TODO diff name head/tail since conflicts w/ unix
; TODO support negative numers like unix tail/head to count from end backwards
(def (head lst n)
  "First n elements of list"
  (cond
    ((<= n 0) Nil)
    ((atom? lst)
     (error 'name "index of list out of bounds"))
    (else (cons (car lst) (head (cdr lst) (- n 1))))))

(def (tail lst n)
  "Last n elements of list"
  (cond
    ((<= n 0) lst)
    ((atom? lst)
     (error 'tail "index of list out of bounds"))
    (else (tail (cdr lst) (- n 1)))))

(def (count elem lst) ; TODO swap arg order?
  (cond ((nil? lst) 0)
        ((atom? lst) (error 'count "expected proper list"))
        ((= elem (car lst)) (+ 1 (count elem (cdr lst))))
        (else (count elem (cdr lst)))))
        ; (else (Binary((elem = car(lst))) + count(elem cdr(lst))))

(def (apply proc args) ; TODO many args
  "Run procedure with given arguments list"
  (eval (map @(list 'quote it) ; prevent proc and args from being evaluated twice
             (cons proc args))))

; TODO rename to foreach (for), swap proc and lst
; TODO many lsts for proc w/ multi arguments, used for index
; [lines 0..len(lines)] |> foreach (line num) => println(num ": " line)
(def (map proc lst)
  "Return new list created by applying procedure to each element of the input list"
  (if pair?(lst)
    (cons (proc (car lst))
          (map proc (cdr lst)))
    Nil))

(def (convert from to lst)
  "Convert every member from of list into to"
  (map @(if (= from it) to it) lst))

; TODO assoc memp procedure equivalent
(def (assoc key table)
  "Return first list in table where the first element matches the key
  If not found, return nil"
  (cond ((nil? table) Nil)
        ((= key (caar table)) (car table))
        (else (assoc key (cdr table)))))

(def (filter proc lst)
  "Only keep elements of list where applying proc returns true
  Also see: keep, remove, member, memp"
  (cond
    ((not (pair? lst)) Nil)
    ((proc (car lst)) (cons (car lst) (filter proc (cdr lst))))
    (else (filter proc (cdr lst)))))

; TODO keep* remove*
(def (keep elem lst)
  "Return list with only elements matching elem
  Also see: filter, remove"
  (filter @(= elem it) lst))

(def (remove elem lst)
  "Return list without elements matching elem
  Also see: filter, keep"
  (filter @(/= elem it) lst))

(def (memp proc lst)
  "Return list of elements after first time procedure applied to each is not nil
  Also see: member, filter"
  (cond ((nil? lst) Nil)
        ((proc (car lst)) lst)
        (else (memp proc (cdr lst)))))

(def (member elem lst)
  "Return list of elements after first matching elem
  Also see: memp, filter"
  (memp @(= elem it) lst))

(def (everyp? proc lst)
  "Return boolean if every element in list passes proc"
  (if (pair? lst)
    (if (proc (car lst))
      (everyp? proc (cdr lst))
      False)
    True))

(def (every? elem lst)
  "Return boolean if every element in list is equal to elem"
  (everyp? @(= elem it) lst))

; TODO rewrite, optimize tco?
(def (compose . procs)
  "Create function made from chaining procedures given"
  (cond
    ((nil? procs) (Func x x))
    ((nil? (cdr procs)) (car procs))
    (else
      (Func x
        ((car procs) (apply (apply compose (cdr procs)) x))))))

(def (reverse lst)
  "Reverse order of list"
  (recur f ((in lst) (out Nil))
    (if (pair? in)
      (f (cdr in) (cons (car in) out))
      out)))

; TODO accept many lists to append
(def (append x y)
  "Append list y to end of list x"
  (cond
    ((pair? x) (cons (car x) (append (cdr x) y)))
    ((nil? x) y)
    ; (else (cons x y)) ; support appending non lists (for flatmap)
    (else (error 'append "expected proper list"))))

; TODO zip to proper pairs (def zip' (zip args (nil list)))
(def (zip x y) ; TODO many args to create longer pairs
  "Create list of pairs made up of elements of both lists"
  (cond ((and (nil? x) (nil? y)) Nil)
        ((or (nil? x) (nil? y)) (error 'zip "given lists of unequal length"))
        ((and (pair? x) (pair? y))
         (cons (cons (car x) (car y))
               (zip (cdr x) (cdr y))))))

;;; Stacks

(def (push stack val)
  "Add value to front of stack
  Also see: pop, peek"
  (cons val stack))

(def (pop stack)
  "Get stack without first element
  Also see: push, peek"
  (if stack
    (cdr stack)
    (error 'pop "Improper stack: " stack)))

(def (peek stack)
  "Get value at front of stack
  Also see: push, pop"
  (if (pair? stack)
    (car stack)
    (error 'peek "Improper stack: " stack)))

(def (swap stack)
  "Get stack where the first 2 values are swapped
  Also see: push, pop, peek"
  (let (x: (peek stack)
        y: (peek (pop stack)))
    (push (push (pop (pop stack)) x) y)))
