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
      "'expr"
      "Return expression unevaluated"
      "  Can be shortened with the ' prefix"
      "  Also see quote, unquote, and unquote-splice")
    (Void
      "Void"
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
    (Func
      "(Func args . body)"
      "(Func body)"
      "Create anonymous function"
      "  If only body is given, create function with one argument labeled 'it'")
    (Macro
      "(Macro args . body)"
      "Create anonymous macro, arguments are not evaluated before given to body"
      "  If only body is given, create macro with one argument labeled 'it'")
    (def
      "(def var . val)"
      "Define new variable with value"
      "  If value is not given, define a self-evaluating symbol"
      ""
      "(def (func . args) . body)"
      "Create new function with arguments list and body list")
    (set!
      "(set! var val)"
      "Change the variable var to val")
    (undefine!
      "(undefine! var)"
      "Remove variable from environment")
    (defined?
      "(defined? var)"
      "Return boolean on if variable is defined in environment")
    (load
      "(load lib)"
      "Loads the library given as a string")
    (error
      "(error func msg)"
      "Throw error, print message with function name given as symbol")
    (version
      "(version)"
      "Tisp's version number as a string")))

(def (docstr-add! . docstrs)
  "Add given docstrs to the global doc string registry"
  ; FIXME for some reason evals docstrs as funcs
  ; `(def docstr-reg (append docstr-reg ,docstrs))
  )

(def (doc proc)
  "Get documentation of procedure"
  (unless (procedure? proc)
    (error 'doc "documentation only exists for procedures"))
  (def (lookup proc)
    (let (docstrs: (assoc proc::name docstr-reg))
      (if docstrs
        (map println (cdr docstrs))
        (error 'doc proc::name ": no documentation found"))))
  (if (or (function? proc) (macro? proc))
    (let (docstr: (car proc::body))
      (println (cons proc::name proc::args))
      (if (string? docstr)
        (println docstr)
        (lookup proc)))
    (lookup proc))
  Void)
