; TODO replace w/ rec, append with def docstr-reg docstr-reg{func: "doc string"}
(def docstr-reg
  '((car
      "car(lst)"
      "First element of list")
    (cdr
      "cdr(lst)"
      "Rest of list after first element, or second element of pair")
    (cons
      "cons(a d)"
      "Create new pair with a car of a and cdr of d")
    (quote
      "quote(expr)"
      "'expr"
      "Return expression unevaluated"
      "  Can be shortened with the ' prefix"
      "  Also see: quasiquote, unquote, and unquote-splice")
    (eval
      "eval(expr)"
      "Evaluate expression"
      "  Can be dangerous to use, as arbitrary code might be executed if the input"
      "  comes from an untrusted source"
      "  Also see: apply")
    (=
      "(= . vals)"
      "Return boolean depending on if multiple values are all equal")
    (cond
      "cond . (expr . body)"
      "Evaluates and returns first body with a true conditional expression"
      "  Also see: if, when, unless")
    (do
      "do . body"
      "Evaluate each expression in body, returning last"
      "  Also see: do0")
    (typeof
      "typeof(val)"
      "Get string stating the argument's type")
    (procprops
      "procprops(prop)"
      "Get record of properties for given procedure"
      "  Includes name, args, and body for functions and macros"
      "  Only has name for builtin primitives and special forms")
    (Func
      "Func args . body"
      "Func body"
      "Create anonymous function"
      "  If only body is given, create function with one argument labeled 'it'"
      "  Also see: def")
    (Macro
      "Macro args . body"
      "Macro body"
      "Create anonymous macro, arguments are not evaluated before given to body"
      "  If only body is given, create macro with one argument labeled 'it'"
      "  Also see: defmacro")
    (def
      "def var . val"
      "Define new variable with value"
      "  If value is not given, define a self-evaluating symbol"
      ""
      "def func(. args) . body"
      "Create new function with arguments list and body list")
    (undefine!
      "undefine!(var)"
      "Remove variable from environment")
    (defined?
      "defined?(var)"
      "Return boolean on if variable is defined in environment")
    (error
      "error(func msg)"
      "Throw error, print message with function name given as symbol")
    (quit
      "quit"
      "Exit REPL, equivalent to Ctrl-D")
    (Str
      "Str(. vals)"
      "Convert all values into single string")
    (Sym
      "Sym(. vals)"
      "Convert all values into single symbol")
    (strformat
      "f\"str\""
      "Perform interpolation on explicit string"
      "  Evaluate expression inside curly braces"
      "  Double curly braces are not evaluated, inserting a single one."
      "  Equivalent to strformat(\"str\"")))

; TODO print func args even if no docstr
(def (doc proc)
  "Get documentation of procedure"
  (unless (procedure? proc)
    (error 'doc "documentation only exists for procedures"))
  (def (lookup proc) ; TODO allow for non proc lookup by make doc macro
    (let (docstrs: (assoc procprops(proc)::name docstr-reg))
      (if docstrs
        (map println (cdr docstrs))
        (error 'doc procprops(proc)::name ": no documentation found"))))
  (if (or (function? proc) (macro? proc))
    (let (docstr: (car procprops(proc)::body))
      (unless (= procprops(proc)::name 'quit)
        (println procprops(proc)::name (or procprops(proc)::args "()")))
      (if (string? docstr)
        (println docstr)
        (lookup proc)))
    (lookup proc))
  Void)
