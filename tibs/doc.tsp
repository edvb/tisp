(define docstr-reg
  '((car
      "(car lst)"
      "return first element of list")
    (cdr
      "(cdr lst)"
      "return rest of list after first element")
    (cons
      "(cons a d)"
      "create new pair with a car of a and cdr of d")
    (quote
      "(quote expr)"
      "return expression unevaluated")
    (void
      "(void)"
      "return void type")
    (eval
      "(eval expr)"
      "evaluate expression, can be dangerous to use in practice")
    (=
      "(= . vals)"
      "return boolean depending on if multiple values are all equal")
    (cond
      "(cond . (expr . body))"
      "conditional statement")
    (typeof
      "(typeof val)"
      "return a string stating the argument's type")
    (get
      "(get val prop)"
      "get the property of the given value depending on its type")
    (lambda
      "(lambda args . body)"
      "create anonymous function")
    (macro
      "(macro args . body)"
      "create anonymous macro")
    (define
      "(define var . val)"
      "(define (func . args) . body)"
      "creates new variable with value, or create new function if argument list given"
      "if value for variable is not given, make it a self-evaluating symbol")
    (set!
      "(set! var val)"
      "change the variable var to val")
    (load
      "(load lib)"
      "loads the library given as a string")
    (error
      "(error func msg)"
      "throw error, print message with function name given as symbol")
    (version
      "(version)"
      "return string of tisp's version number")))

(define (doc proc)
  "get documentation of function supplied by its doc string"
  (unless (procedure? proc)
    (error 'doc "documentation only exists for procedures"))
  (define (lookup proc)
    (recur f ((docstr docstr-reg))
      (cond ((nil? docstr)
             (error 'doc (get proc 'name) ": no documentation found"))
            ((= (caar docstr) (get proc 'name))
             (map disp (cdar docstr)))
            (else (f (cdr docstr))))))
  (if (or (function? proc) (macro? proc))
    (let ((docstr (car (get proc 'body))))
      (if (string? docstr)
        (disp (cons (get proc 'name) (get proc 'args)) "\n"
              docstr)
        (lookup proc)))
    (lookup proc))
  (void))
