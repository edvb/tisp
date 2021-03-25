# tisp \- tiny lisp

Tisp programming language interpreter.  Read and evaluate all files in order given, if file name
is `-` read from `stdin`. If no files are supplied launch REPL.

## Options

#### -c COMMAND

Read *COMMAND* as a line of Tisp code and print result

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
(1 . 2)
```

Alternatively you can pass a file name which will be opened and run, outputting
the result before exiting.

```
$ echo '((lambda (x) (+ x 1)) 10)' > inc.tsp
$ tisp inc.tsp
11
```

Commands can also be piped directing into Tisp through the command line.

```
$ echo '(= "foo" "foo")' | tisp
t
```

Or given directly to Tisp as an argument:

```
$ tisp -c "(reverse '(1/2 1/4 1/8 1/16))"
(1/16 1/8 1/4 1/2)
```

## Author

Ed van Bruggen <ed@edryd.org>

## See Also

tisp(7)
tsp(1)

See project at <https://edryd.org/projects/tisp>

View source code at <https://git.edryd.org/tisp>

## License

zlib License
