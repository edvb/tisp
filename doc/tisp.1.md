# tisp \- tiny lisp

[![Build Status](https://travis-ci.org/edvb/tisp.svg)](https://travis-ci.org/edvb/tisp)

Tisp is a tiny, terse, and transparent lisp library designed to be lightweight
and easy to embedded in other programs. Simply drop the `tisp.c` and `tisp.h`
files into your project in order to use the necessary functions for your
program. An example command line interpreter is provided in `main.c`. Tisp is a
single value name language (lisp-1) which tries to be unambiguous and focus
on simplicity.  Much of the language is defined with Tisp itself leaving the C
code as short as possible.

## Options

Read and evaluate all files in order given, if file name is `-` read from
`stdin`. If no files are supplied launch REPL.

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
(cons 1 2)
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

## Language

Tisp is mainly based off of scheme, with minor features borrowed from other
lisps and scripting languages.

### General

#### Comments

Single line comments with a semicolon, **eg** `(cons 1 2) ; ingnored by tisp until new line`.

### Types

#### Nil

Nil, null, empty, or false, represented as an empty list, **eg** `()`, `nil` `false`.

#### Integers

Whole real numbers, positive or negative with optional `+` or `-` prefixes
repressively. Also supports scientific notation with a capital or lowercase
`e`. The exponent needs to be an positive integer, it can also be negative but
that would just round to zero.  **eg** `1`, `-48`, `+837e4`, `3E-2`.

#### Decimals

Floating point numbers, also know as decimals, are numbers followed by a period
and an optional mantissa. Like integers they can be positive or negative with
scientific notation, but still need an integer as an exponent. **eg** `1.`,
`+3.14`, `-43.00`, `800.001e-3`.

#### Rationals

Fraction type, a ratio of two integers. Similar rules apply for numerator and
dominator as integers (real positive or negative), except for scientific
notation. Will simplify fraction where possible, and will through error
on division by zero. **eg** `1/2`, `4/3` `-1/2`, `01/-30`, `-6/-3`.

#### Strings

String of characters contained inside two double quotes. Any character is
valid, including actual newlines, execpt for double quotes and backspace which
need to be escaped as `\"` and `\\` respectively. Newlines and tabs can also be
escaped with `\n` and `\t` **eg** `"foo"`, `"foo bar"`, `"string \"quoted\""`,
`"C:\\windows\\path"` `\twhite\n\tspace`.

#### Symbols

Case sensitive symbols which are evaluated as variable names. Supports lower and
upper case letters, numbers, as well as the characters `_+-*/\\^=<>!?@#$%&~`.
First character can not be a number, if the first character is a `+` or `-`
then the second digit cannot be a number either. Unlike all the previously
listed types, symbols are not self evaluating, but instead return the value
they are defined to. Throws an error if a symbol is evaluated without it being
previously assigned a value. **eg** `foo`, `foo-bar`, `cat9`, `+`, `>=`,
`nil?`.

#### Lists

Lists composed of one or more elements of any type, including lists themselves.
Expressed with surrounding parentheses and each element is separated by
whitespace. When evaluated the procedure found at the first element is run with
the rest of the elements as arguments. Technically list is not a type, but
simply a nil-terminated chain of nested pairs. A pair is a group of two and
only two elements, normally represented as `(a . b)`, with `a` being the first
element (car/first) and `b` being the second element (cdr/rest). For example
`(a b c)` is equivilent to `(a . (b . (c . ())))`, but it is often easier to
express them the first way syntactically. To create an improper list (not
nil-terminated list) the dot notation can be used, `(1 2 3 . 4)`.

#### Functions

Lambda functions created within Tisp itself. Called using list syntax where the
first element is the function name and any proceeding elements are the
arguments to be evaluated. For example `(cadr '(1 2 3))` is a list of elements
`cadr` and `'(1 2 3)`. It calls the function `cadr` which returns the 2nd
element of the argument given, in this case a list of size 3, which returns
`2`. **eg** `list`, `not`, `apply`, `map`, `root`, `sqrt`, `print`

#### Primitives

Procedures built in to the language written in C. Called like regular
functions, see primitives section for more details. **eg** `car`, `cond`,
`define`, `write`, `+`

#### Macros

Like lambda functions macros are defined within Tisp and are called in a
similar fashion. However unlike functions and primitives the arguments are not
evaluated before being given to the macro, and the output of a macro is
only evaluated at the end. This means instead of taking in values and returning
values, macros take in code and return code, allowing you to extend the
language at compile time. **eg** `if`, `and`, `or`, `let`, `assert`

### Built-ins

Built in primitives written in C included by default.

#### car

Returns first element of given list

#### cdr

Return rest of the given list, either just the second element if it is a pair,
or another list with the first element removed.

#### cons

Creates a new pair with the two given arguments, first one as the car, second
as the cdr. Can be chained together to create a list if ending with `nil`.

#### quote

Returns the given argument unevaluated.

#### void

Returns nothing. Used to insert a void type in a list or force a function not
to return anything.

#### eval

Evaluates the expression given, can be dangerous to use in practice. In general
`apply` should be used instead.

#### =

Tests if multiple values are all equal. Returns `nil` if any are not, and `t`
otherwise.

#### cond

Evaluates each expression if the given condition corresponding to it is true.
Runs through all arguments, each is a list with the first element as the
condition which needs to be `t` after evaluated, and the rest of the list is
the body to be run if and only if the condition is met. Used for if/elseif/else
statements found in C-like languages and `if`,`when`,`unless`,`switch` macros
in Tisp.

#### type

Returns a string stating the given argument's type. Used to create `type?`
individual functions.

#### lambda

Creates function, first argument is a list of elements representing the symbol
name for any arguments of the new function. Rest of the arguments are code to
be run with the supplied arguments.

#### macro

Similar to lambda, creates anonymous macro with first argument as macro's
argument list and rest as macro's body.

#### define

Create variable with the name of the first argument, with the value of the
second. If name given is a list use the first element of this list as a new
functions name and rest of list as its arguments.

#### set!

Change the value of the of the variable given by the first argument to the
second argument. Errors if variable is not defined before.

#### load

Loads the library given as a string.

#### error

Throw error, print message given by second argument string with the first
argument being a symbol of the function throwing the error.

#### version

Return string of Tisp's version number.

### Differences From Other Lisps

In Tisp there are no boolean types, much like common lisp, true is represented
by the self evaluating symbol `t` and false is nil, represented as `()`, an
empty list. `nil` and `false` are also mapped to `()`.

Tisp also only has one builtin equality primitive, `=`, which tests integers,
symbols, strings, and objects which occupy the same space in memory, such as
primitives.

Symbols are case sensitive, unlike many other older lisps, in order to better
interface with modern languages.

Tisp is single value named, so procedures share the same namespace as
variables, removing the need for common lisp's `defunc` vs `defvar`, `let` vs
`flet`, and redundant syntax for getting the function from a symbol.

## Author

Ed van Bruggen <ed@edryd.org>

## See Also

See project page at <https://edryd.org/projects/tisp.html>

View source code at <https://git.edryd.org/tisp>

## License

zlib License
