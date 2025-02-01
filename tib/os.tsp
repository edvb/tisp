(def docstr-reg (append docstr-reg
  '((cd!
      "cd!(dir)"
      "Change directory")
    (pwd
      "pwd()"
      "String of current working directory")
    (exit!
      "exit!(status)"
      "Exit program with status code of given integer"
      "  Zero for success, non-zero for failure")
    (now
      "now()"
      "Number of seconds since 1970 (unix time stamp)")
    (time
      "time(expr)"
      "Time in milliseconds it took to run expression"))))

; TODO % stack of prev res, %% = fst %
(def (repl)
  "Read, evaluate, print, loop
  To exit enter quit or press CTRL-D"
  (print "> ")
  (let (expr: (parse (read)))
    (unless (= expr 'quit)
      ; TODO push ans to stack of outputs
      (let (ans: (eval expr)) ; env{ :% %%: fst(%) }
        (unless (void? ans)
          (displayln ans))
        (repl)))))

(def (repl-simple)
  "Simple REPL interface, only dependencies are builtin functions, no tisp code
  To exit enter quit or press CTRL-D
  See repl for more advanced REPL with prettier printing"
  (write 'stdout Nil "> ")
  ((Func (expr)
     (cond
       ((= expr 'quit))
       (else
         write('stdout Nil (eval expr) "\n")
         (repl-simple))))
  (parse (read))))
