(define (repl)
  (print "> ")
  (let ((expr (parse (read))))
    (unless (and (pair? expr) (= (car expr) 'quit))
      (let ((ans (eval expr)))
        (unless (void? ans)
          (disp-string ans)))
      (repl))))

;; simple repl
(define (repl-simple)
  (print "> ")
  (print (eval (parse (read))))
  (newline)
  (repl-simple))

