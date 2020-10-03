(def (run file) (eval (parse (read file))))
(def (print . str) (apply write (list* 'stdout Nil str)))
(def (newline . file)
  (if (or (nil? file) (nil? (cdr file)))
    (write (car (or file '(stdout))) file "\n")
    (error 'newline "only zero or one file can be given")))

(def (display . str)
  (map (lambda (s)
         (cond
           ((string? s) (print "\"" s "\""))
           ((true? s)   (print s)) ; don't quote True since it's self evaluating
           ((symbol? s) (print "'" s))
           ((pair? s)   (print "'" s))
           (else        (print s))))
       str))
(def (displayln . str) (apply display str) (newline))
(def (println . str)   (apply print str) (newline))
