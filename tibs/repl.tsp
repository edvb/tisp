(define (repl)
  (print "> ")
  ((lambda (expr)
     (if (= expr 'quit)
       (void)
       (do
         ((lambda (ans)
            (cond ((not (void? ans))
                   (disp-string ans))))
          (eval expr))
         (repl))))
   (read)))

(repl)
