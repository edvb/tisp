# tisp \- tiny lisp

Tisp programming language default interpreter.  Read and evaluate all files in order given, if
file name is `-` read from `stdin`. If no files are supplied launch the read-evaluate-print-loop
(REPL) for interactive running of commands.

## Options

#### -e EXPRESSION

Read *EXPRESSION* as a line of Tisp code, evaluate, and print result

#### -r

Launch the REPL prompt. Default if no arguments are given. Useful for
loading files and then running REPL after. Equivalent to -e '(repl)'

#### -h

Print help and exit

#### -v

Print version info and exit

## Usage

Run the program from the command line to type a single command and press enter
to see the result.

```
$ tisp
> (cons 1 2)
'(1 . 2)
> (list 1 (+ 1 1) 3 (* 2 2))
'(1 2 4)
```

Alternatively you can pass a file name which will be opened and run, outputting
the result before exiting.

```
$ echo '((Func (x) (+ x 1)) 10)' > inc.tsp
$ tisp inc.tsp
11
```

Commands can also be piped directing into Tisp through the command line.

```
$ echo '(= "foo" "foo")' | tisp
True
```

Or given directly to Tisp as an argument:

```
$ tisp -e "(reverse '(1/2 1/4 1/8 1/16))"
(1/16 1/8 1/4 1/2)
```

## See Also

tisp(7)

See project at <https://edryd.org/projects/tisp>

View source code at <https://git.edryd.org/tisp>

## Author

Edryd van Bruggen <ed@edryd.org>

## License

zlib License
