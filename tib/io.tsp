def docstr-reg (append docstr-reg
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
      "Loads the library given as a string")))

def newline(. file) ; TODO space and tab functions, accept number of instead
  if (or nil?(file) nil?(cdr(file)))
    write car((or file '(stdout))) file "\n"
    error 'newline "only zero or one file can be given"

def display(. vals)
  "Convert values to string which evaluates to itself
  Used when displaying output of repl"
  apply Str
    map
      @(cond
          string?(it): (Str "\"" it "\"") ; TODO insert escape characters for non-printable
          true?(it):   (Str it) ; don't quote True symbol since it's self evaluating
          symbol?(it): (Str "'" it) ; TODO if contains non symbol char print as explicit sym syntax
          pair?(it):
            (Str
              "["
              display(car(it))
              (or (walk
                    Func((x) (if pair?(x)
                               Str(" " display(car(x)))
                               Str(" . " display(x))))
                    cdr(it))
                  "")
              "]")
          record?(it):
            (Str
              "{ "
              (or (map Func((x) Str(car(x) ": " display(cdr(x)) " ")) records(it))
                  "")
              "}")
         else: (Str it))
      vals

def print(. str)      write('stdout Nil . str)
def println(. str)    print(. str) newline()
def printerr(. str)   write('stderr Nil . str)
def printerrln(. str) (printerr . str) newline()
