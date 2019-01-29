# tisp \- tiny lisp

[![Build Status](https://travis-ci.org/edvb/tisp.svg)](https://travis-ci.org/edvb/tisp)

tisp is a tiny lisp library designed to to be lightweight and easy to embedded
in other programs. Simply drop the `tisp.c` and `tisp.h` files into your
project and include the header file in order to use the necessary functions for
your program. An example command line interpreter is provided in `main.c`.

## Options

#### -h

Print help and exit

#### -v

Print version info and exit

## Usage

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
$ echo '((lambda (x) (+ x 1)) 10)' > inc.lisp
$ tisp inc.lisp
11
```

Commands can also be piped directing into tisp through the command line.

```
$ echo '(= "foo" "foo")' | tisp
t
```

## Language

tisp is mainly based off of scheme, with minor features borrowed from other
lisps.

### General

#### Comments

Single line comments with a semicolon, **eg** `(cons 1 2) ; ingnored by tisp until new line`.

### Types

#### Nil

Nil, null, empty, or false, represented as an empty list, **eg** `()`.

#### Integers

Whole real numbers, positive or negative with optional `+` or `-` prefixes
repressively. Also supports scientific notation with a capital or lowercase
`e`. The exponent also needs to be integer which can be positive or negative.
**eg** `1`, `-48`, `+837e4`, `3E-2`.

#### Floating Pointing

Floating point numbers, as know as decimals, are integers followed by a period
and an optional integer with leading integers. Like integers can be positive or
negative with scientific notation, but still need an integer as an exponent. **eg**
`1.`, `+3.14`, `-43.00`, `800.001e-3`.

#### Rationals

Fraction type, a ratio of two integers. Similar rules apply for numerator and
dominator as integers (real positive or negative), expect for scientific
notation. Will try to simplify fraction where possible, and will through error
on division by zero. **eg** `1/2`, `4/3` `-1/2`, `01/-30`, `-6/-3`.

#### Strings

String of characters contained inside two double quotes. **eg** `"foo"`, `"foo bar"`.

#### Symbols

Case sensitive symbols which can be used as variable names. Supports lower and
upper case letters, numbers, as well as the characters `_+-*/=<>?`. First
character can not be a number, if the first character is a `+` or `-` then the
second digit cannot be a number either. Unlike all the previously listed types,
symbols are not self evaluating, but instead return the value they are defined
to. Throws an error if a symbol is evaluated without it being previously
assigned a value. **eg** `foo`, `foo-bar`, `cat9`, `+`, `>=`, `nil?`.

#### Lists

Lists composed of one or more element of any other types, including lists them
selves. Expressed with surrounding parentheses and each element is separated by
white space. When evaluated runs the first element as function with the rest of
the elements as arguments. Technically list is not a type, but simply a
nil-terminating chain of nested pairs. A pair is a group of two and only two
elements, normally represented as `(a . b)`, with `a` being the first element
(car) and `b` being the second element (cdr). for example `(a b c)` is actually
`(a . (b . (c . ())))`. But it is often easier just to think of them as lists.

#### Functions

Lambda functions created within tisp itself. Called using list syntax where the
first element is the function name and any proceeding elements are the
arguments. For example `(cadr '(1 2 3))` is a list of elements `cadr` and `'(1
2 3)`. It calls the function `cadr` which returns the 2nd element of the first
argument given, here a list of size 3. In this case it return a `2`.

#### Primitives

Functions built in to the language written in C. Called like regular functions,
see primitives section for more details.

### Primitives

Built in primitives included by default.

#### car

Returns first element of given list

#### cdr

Return rest of the given list, either just the second element if it is of size
2 or a pair, or a new list with the first element removed.

#### cons

Creates a new pair with the two given arguments, first one as the car, second
as the cdr.

#### quote

Returns the given argument unevaluated.

#### void

Returns nothing. Used to insert a void type in a list or force a function not
to return anything.

#### begin

Executes all of its arguments and returns the value of the last expression.

#### eval

Evaluates the expression given.

#### =

Tests if multiple values equal. Returns nil if any are not, and `t` otherwise.

#### cond

Evaluates each expression if the given condition corresponding to it is true.
Runs through any arguments, each is a list with the first element as the
condition which needs to be `t` after evaluated, and the second element is the
expression to be run if and only if the condition is met.

#### type

Returns a string stating the given argument's type.

#### lambda

Creates function, first argument is a list of elements representing the symbol
name for any arguments for the new function. Next argument is code to be run
with the supplied arguments.

#### define

Create variable with the name of the first argument, with the value of the
second.

#### load

Loads the library, given as a string.

### Differences From Other Lisps

In tisp there are no boolean types, much like common lisp, true is represented
by the self evaluating symbol `t` and false is nil, represented as `()`, an
empty list.

tisp also only has one equality primitive, `=`, which tests integers, symbols,
strings, and objects which occupy the same space in memory, such as primitives.
It also accepts any number of arguments to compare.

Symbols are also case sensitive following the Unix way, unlike many other lisps.

## Author

Ed van Bruggen <edvb@uw.edu>

## See Also

See project page at <https://edryd.org/projects/tisp.html>

View source code at <https://git.edryd.org/tisp>

## LICENSE

zlib License
