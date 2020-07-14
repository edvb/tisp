(define (run file) (eval (parse (read file))))
(define (print . str) (apply write (list* 'stdout nil str)))
(define (newline . file)
  (if (or (nil? file) (nil? (cdr file)))
    (write (car (or file '(stdout))) file "\n")
    (error 'newline "only zero or one file can be given")))

(define (display . str)
  (map (lambda (s)
         (cond
           ((string? s) (print "\"" s "\""))
           ((true? s)   (print s)) ; don't print 't since it's self evaluating
           ((symbol? s) (print "'" s))
           ((pair? s)   (print "'" s))
           (else        (print s))))
       str))
(define (displayln . str) (apply display str) (newline))
(define (println . str)   (apply print str) (newline))
