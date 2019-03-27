(define (repl)
  (print "> ")
  ((lambda (expr)
     (cond ((= expr 'quit) (newline))
           (t (do
                ((lambda (ans)
                   (cond ((not (void? ans))
                          (disp ans))))
                 (eval expr))
                (repl)))))
   (read)))

(repl)
