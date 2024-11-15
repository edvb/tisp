# tisp \- tiny lisp

Tisp is a tiny, terse, and transparent lisp programming language designed to be
lightweight, modular, extendable, and easy to embedded into other programs.
Tisp is a functional language which tries to be unambiguous and focus on
simplicity.  Much of the language is defined with Tisp itself leaving the C
code as short as possible and maximally orthogonal.

To include Tisp in your project, simply drop the `tisp.c`, `tisp.h`,
`tibs.tsp.h`, and any lib files into your project to use the necessary
functions for your program. An example command line interpreter is provided in
`main.c`.

The language is still in active development and not yet stable. Until the
`v1.0` release expect non-backwards compatible changes in minor releases.

## Language

Tisp is a strong dynamically typed language (with a powerful first-class static
type system in the works) inspired mostly by Scheme, with ideas from Python,
Haskell, Julia, and Elm as well.

### General

#### Comments

Single line comments with a semicolon, **eg** `(cons 1 2) ; ingnored by tisp
until new line`.

### Types

#### Nil

Nil, null, empty, or false, represented as an empty list, **eg** `()`, `nil`
`false`.

#### Integers

Whole real numbers, positive or negative with optional `+` or `-` prefixes.
Also supports scientific notation with an upper or lowercase `e`. The exponent
should be a positive integer, it can also be negative but that would just round
to zero for integers.  **eg** `1`, `-48`, `+837e4`, `3E2`.

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

#### Booleans

In Tisp all values except `Nil` (the empty list `()`) are truthy. The symbols
`True` and `False` are also defined for explicit booleans, `False` being mapped
to `Nil`. Functions which return a boolean type should end with `?`.
**eg** `pair?`, `integer?`, `even?`

#### Void

Returns nothing. Used to insert a void type in a list or force a function not
to return anything.

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
`def`, `write`, `+`

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

#### eval

Evaluates the expression given. Can be dangerous to use as arbitrary code could
be executed if the input is not from a trusted source. In general `apply`
should be used when possible.

#### =

Tests if multiple values are all equal. Returns `Nil` if any are not, and `True`
if they are.

#### cond

Evaluates each expression if the given condition corresponding to it is true.
Runs through all arguments, each is a list with the first element as the
condition which needs to be `True` after evaluated, and the rest of the list is
the body to be run if and only if the condition is met. Used for if/elseif/else
statements found in C-like languages. Also see `if`,`when`,`unless`,`switch`
macros in Tisp.

#### typeof

Returns a string stating the given argument's type.

#### Func

Creates anonymous function, first argument is a list of symbols for the names
of the new function arguments. Rest of the arguments to Func is the body of
code to be run when the function is called. Also see `def`.

#### Macro

Functions which operate on syntax expressions, and return syntax. Similar to
Func, Macro creates anonymous macro with first argument as macro's argument
list and rest as macro's body. Unlike functions macros do not evaluate their
arguments when called, allowing the expressions to be transformed by the macro,
returning a new expression to be evaluated at run time. Also see `defmacro`

#### def

Create variable with the name of the first argument, with the value of the
second. If name given is a list use the first element of this list as a new
functions name and rest of list as its arguments. If only variable name is
given make it a self evaluating symbol.

#### set!

Change the value of the of the variable given by the first argument to the
second argument. Errors if variable is not defined before.

#### undefine!

Remove variable from environment. Errors if variable is not defined before.

#### defined?

Return boolean on if variable is defined in the environment.

#### load

Loads the library given as a string.

#### error

Throw error, print message given by second argument string with the first
argument being a symbol of the function throwing the error.

#### version

Return string of Tisp's version number.

### Differences From Other Lisps

By default Tisp's output is valid Tisp code, fully equivalent to the evaluated
input. Lists and symbols are quoted (`(list 1 2 3) => '(1 2 3)`), errors are
comments. The only exception is procedural types which will be fixed soon. To
print value as valid Tisp code use `display` and `displayln`, to get a plain
output use `print` and `println`.

Tisp only has one builtin equality primitive, `=`, which tests integers,
symbols, strings, and objects which occupy the same space in memory, such as
primitives.

Symbols are case sensitive, unlike many other older lisps, in order to better
interface with modern languages.

Tisp is single value named, so procedures share the same namespace as
variables. This way functions are full first class citizens. It removes the
need for common lisp's `defunc` vs `defvar`, `let` vs `flet`, and redundant
syntax for getting the function from a symbol.

## Author

Ed van Bruggen <ed@edryd.org>

## See Also

tisp(1)
tsp(1)

See project at <https://edryd.org/projects/tisp>

View source code at <https://git.edryd.org/tisp>

## License

zlib License
