# tisp \- tiny lisp

[![Build Status](https://travis-ci.org/edvb/tisp.svg)](https://travis-ci.org/edvb/tisp)

Tiny lisp implementation, still in heavy development, barely even "useable."
Designed to be scheme-like, ie very minimalist.

## OPTIONS

#### -h

Print help and exit

#### -v

Print version info and exit

## USAGE

Run the program from the command line to launch the REPL, type a command and
press enter to see the result.

```
$ tisp
> (+ (+ 1 2) 3)
6
> (+ 1 2 3)
6
```

Alternatively you can pass a file name which will be opened and run, outputting
the result before exiting.

```
$ echo '(+ 1 2 3)' > add.lisp
$ tisp add.lisp
6
```

Commands can also be piped directing into tisp through the command line.

```
$ echo '(+ 1 2 3)' | tisp
6
```

## AUTHOR

Ed van Bruggen <edvb@uw.edu>

## SEE ALSO

See project page at <https://edryd.org/projects/tisp.html>

View source code at <https://git.edryd.org/tisp>

## LICENSE

zlib License
