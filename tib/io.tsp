(def docstr-reg (append docstr-reg
  '((write
      "(write file append? . vals)"
      "Writes to file all values"
      "  File can be file path give as string or the symbols 'stdout or 'stderr"
      "  Append file if append? is true, otherwise override")
    (read
      "(read . file)"
      "Return string read from optional file, if not given read from stdin")
    (parse
      "(parse string)"
      "Parse Tisp object from given string"))))

(def (run file) (eval (parse (read file))))
(def (print . str) (apply write (list* 'stdout Nil str)))
(def (newline . file) ; TODO space and tab functions, accept number of instead
  (if (or (nil? file) (nil? (cdr file)))
    (write (car (or file '(stdout))) file "\n")
    (error 'newline "only zero or one file can be given")))

(def (display . str)
  (map @(cond
          ((string? it) (print "\"" it "\""))
          ((true? it)   (print it)) ; don't quote True since it's self evaluating
          ((symbol? it) (print "'" it))
          ((pair? it)   (print "'" it))
          (else         (print it)))
       str)
  Void)
(def (displayln . str) (apply display str) (newline))
(def (println . str)   (apply print str) (newline))
