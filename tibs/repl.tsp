(define (repl)
  (print "> ")
  ((lambda (exp)
     (cond ((= exp 'quit) (print (newline)))
           (t (begin
                ((lambda (eval-exp)
                   (cond ((not (void? eval-exp))
                          (disp eval-exp))))
                 (eval exp))
                (repl)))))
   (read)))

(repl)
