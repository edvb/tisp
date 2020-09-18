(def (repl)
  (print "> ")
  (let ((expr (parse (read))))
    (unless (and (pair? expr) (= (car expr) 'quit))
      ; TODO push! ans to stack of outputs
      (let ((ans (eval expr)))
        (unless (void? ans)
          (displayln ans))
        (repl)))))

;; simple repl, only requires io.c tib
(def (repl-simple)
  (write 'stdout nil "> ")
  (write 'stdout nil (eval (parse (read))) "\n")
  (repl-simple))
