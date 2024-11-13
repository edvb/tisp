(docstr-add!
  '((cd!
      "(cd! dir)"
      "Change directory")
    (pwd
      "(pwd)"
      "String of current working directory")
    (now
      "(now)"
      "Number of milliseconds since 1970 (unix time stamp)")
    (time
      "(time expr)"
      "Time in milliseconds it took to run expression")))

(def (repl)
  "Read, evaluate, print, loop
  To exit enter (quit) or CTRL-D"
  (print "> ")
  (let ((expr (parse (read))))
    (unless (and (pair? expr) (= (car expr) 'quit))
      ; TODO push! ans to stack of outputs
      (let ((ans (eval expr)))
        (unless (void? ans)
          (displayln ans))
        (repl)))))

(def (repl-simple)
  "Simple REPL interface, only requires io.c tib, no tisp code
  See repl for more advanced REPL"
  (write 'stdout Nil "> ")
  (write 'stdout Nil (eval (parse (read))) "\n")
  (repl-simple))
