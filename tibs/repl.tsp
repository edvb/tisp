(define (repl)
  (print "> ")
  (let ((expr (parse (read))))
    (unless (and (pair? expr) (= (car expr) 'quit))
      (let ((ans (eval expr)))
        (unless (void? ans)
          (disp-string ans))
        (repl)))))

;; simple repl, only requires io c library
(define (repl-simple)
  (write 'stdout "> " (eval (parse (read))) "\n")
  (repl-simple))
