(def docstr-reg (append docstr-reg
  '((write
      "write(file append? . vals)"
      "Write all values to file"
      "  File can be file path give as string or the symbols 'stdout or 'stderr"
      "  Append file if append? is true, otherwise override")
    (read
      "read(. file)"
      "Return string read from file, or stdin if no file is provided")
    (parse
      "parse(string)"
      "Parse Tisp object from given string")
    (load
      "load(lib)"
      "Loads the library given as a string"))))

(def (newline . file) ; TODO space and tab functions, accept number of instead
  (if (or (nil? file) (nil? (cdr file)))
    (write (car (or file '(stdout))) file "\n")
    (error 'newline "only zero or one file can be given")))

(def (display . str)
  (map @(cond
          ((string? it) (print "\"" it "\""))
          ((void? it))
          ((true? it)   (print it)) ; don't quote True since it's self evaluating
          ; TODO if contains non symbol char print as explict sym syntax
          ((symbol? it) (print "'" it))
          ((pair? it)   print("[")
                        display(car(it))
                        (map Func((x) print(" ") display(x)) cdr(it))
                        print("]"))
          ((record? it) print("{ ")
                        (map Func((x) print(car(x) ": ") display(cdr(x)) print(" ")) records(it))
                        print("}"))
          (else         (print it)))
       str)
  Void)
(def (displayln . str) (apply display str) (newline))
(def (print . str)     (write 'stdout Nil . str))
(def (println . str)   (print . str) (newline))
(def printerr(. str)   (write 'stderr Nil . str))
(def printerrln(. str) (printerr . str) (newline))
