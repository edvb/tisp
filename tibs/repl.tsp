(define (repl)
  (print "> ")
  ((lambda (expr)
     (cond ((= expr 'quit) (newline))
           (t (do
                ((lambda (eval-expr)
                   (cond ((not (void? eval-expr))
                          (disp eval-expr))))
                 (eval expr))
                (repl)))))
   (read)))

(repl)
