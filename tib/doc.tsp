(def docstr-reg
  '((car
      "(car lst)"
      "First element of list")
    (cdr
      "(cdr lst)"
      "Rest of list after first element")
    (cons
      "(cons a d)"
      "Create new pair with a car of a and cdr of d")
    (quote
      "(quote expr)"
      "Return expression unevaluated"
      "  Can be shortened with the ' prefix"
      "  Also see quote, unquote, and unquote-splice")
    (Void
      "(Void)"
      "Return nothing"
      "Used to insert a void type in a list or force a function not to return anything")
    (eval
      "(eval expr)"
      "Evaluate expression, can be dangerous to use in practice")
    (=
      "(= . vals)"
      "Return boolean depending on if multiple values are all equal")
    (cond
      "(cond . (expr . body))"
      "Conditional statement")
    (typeof
      "(typeof val)"
      "Get string stating the argument's type")
    (get
      "(get val prop)"
      "Get the property of the given value depending on its type")
    (lambda
      "(lambda args . body)"
      "Create anonymous function")
    (macro
      "(macro args . body)"
      "Create anonymous macro")
    (def
      "(def var . val)"
      "(def (func . args) . body)"
      "Create new variable with value, or create new function if argument list given"
      "  If value for variable is not given, make it a self-evaluating symbol")
    (set!
      "(set! var val)"
      "change the variable var to val")
    (load
      "(load lib)"
      "Loads the library given as a string")
    (error
      "(error func msg)"
      "Throw error, print message with function name given as symbol")
    (version
      "(version)"
      "Tisp's version number as a string")))

(def (doc proc)
  "get documentation of function supplied by its doc string"
  (unless (procedure? proc)
    (error 'doc "documentation only exists for procedures"))
  (def (lookup proc)
    (recur f ((docstrs docstr-reg))
      (cond ((empty? docstrs)
             (error 'doc (get proc 'name) ": no documentation found"))
            ((= (caar docstrs) (get proc 'name))
             (map println (cdar docstrs)))
            (else (f (cdr docstrs))))))
  (if (or (function? proc) (macro? proc))
    (let ((docstr (car (get proc 'body))))
      (println (cons (get proc 'name) (get proc 'args)))
      (if (string? docstr)
        (println docstr)
        (lookup proc)))
    (lookup proc))
  (Void))
