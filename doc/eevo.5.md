# eevo \- easy expression value organizer

eevo is a strong dynamically typed language (with a powerful first-class static
type system in the works) inspired mostly by Scheme, with ideas from Lua,
Python, Haskell, Julia, and Elm as well.

## General

### Expressions

Expressions are the building blocks of eevo scripts, everything in eevo is an
expression. They come in many different kinds, shown in the types section
below.

### Comments

Comments are explanatory notes that are ignored by eevo.
They help document how the code works and, more importantly, why it is done that way.
Comments start with a semicolon (`;`) and continue until the end of the line.

**Example:** `println "Hello World" ; ingnored by eevo until end of line`.

* Double semicolon `;;` denotes documentation comments.
* Triple semicolon `;;;` marks section headers.

*Note*: While many scripting languages use `#`, eevo adopts semicolons
for more convenient keyboard access, encouraging frequent use.

## Types

Types are the nouns of eevo, they describe the different kinds of expressions
that eevo can represent and transform.

### Numbers

#### Integers

Whole numbers with optional `+` or `-` prefixes. Supports scientific notation
with `e` or `E` followed by another integer.

*Note*: The exponent should be a positive integer, it can also be negative but
that would round to zero for integers.

**Examples:** `1`, `-48`, `+837e4`, `3E2`.

#### Decimals

Floating-point numbers, denoted with decimal point. Supports scientific
notation with `e` or `E` followed by an integer.

**Examples:** `1.`, `0.018`, `+3.14`, `-.0054`, `800.001e-3`.

#### Rationals

Fraction type, a ratio of two integers. Same integer rules apply for numerator
and dominator, without scientific notation.

Automatically simplified, and will through error on division by zero.

**Examples:** `1/2`, `4/3`, `-1/12`, `01/-30`, `-6/-3`.

### Booleans

#### True

All values except `Nil` are truthy, including explicit `True`, integers,
strings, non-empty lists, etc.

**Examples:** `True`, `1`, `[8 7 6]`, `"any string"`.

#### Nil

Nil represents false, or an empty list.

**Examples:** `Nil`, `False`, `()`.

### Void

Represents the absence of a value. Used to force functions to return nothing or
as a placeholder in lists.

### Text

#### Strings

String of characters surrounded by double quotes `"`. Any character is
valid, including newlines, except for double quotes and backslash which
need to be escaped as `\"` and `\\` respectively. Newlines and tabs can also be
escaped with `\n` and `\t`

**Examples:** `"foo"`, `"foo bar"`, `"string \"quoted\""`, `"C:\\windows\\path"`,
`"\tstring\twith   white\n\tspace  "`.

#### Symbols

Case sensitive identifiers which are evaluated as variable names.

Unlike other types, symbols are not self-evaluating, they resolve to their
defined value and throw an error if undefined.

Valid characters include lower and upper case letters, numbers, and
`_+-*/\^=<>!?@#$%&~`. They can not start with a number, and if the first
character is a `+` or `-` then the second digit cannot be a number either.

**Examples**: `foo`, `foo-bar`, `cat9`, `+`, `>=`, `nil?`.

### Collective Nouns

The basic types covered so far can be composed in multiple ways:

#### Pairs

A grouping of exactly two expressions in `(fst ... rst)` notation.
`fst` is the first element and `rst` is the second element, either can be any
type (including another pair) and do not have to be the same type.

When evaluated, the first element is a procedure, the rest are its argument,
and the result is the procedure applied to the argument.

**Examples**: `(1 ... 2)`, `(f ... args)`

#### Lists

Ordered sequences of expressions enclosed in parenthesis `( )` or brackets `[
]` separated by white space.
Made from chains of pairs ending with an empty list (**eg** `(1 2 3) = (1 ...
(2 ... (3 ... Nil)))`).

Lists written in parentheses are evaluated as function calls, while those in
brackets produce lists with their inner expressions evaluated
(**eg** `[1/2 2/2 3/2] = (list 1/2 1 3/2)`).

The first element of a list can be placed before the parentheses to
imitate more traditional function call syntax (**eg** `f(x y z) = (f x y z)`).

**Examples**: `(one two three)`, `[proton.mass neutron.mass electron.mass]`,
`split(notes ",")`, `(now)`

#### Improper Lists

Chains of pairs where the last expression is not `Nil`.

**Examples**: `(Fry Leela Bender ... others)`, `[1 2 3 ... 4]`

#### Records

Records are unordered groupings of one or more key-value pairs separated by a
colon and enclosed in curly braces.
Sometimes called dictionaries, hash tables, structs, objects, or maps.

The key is a symbol which maps to a value of any type.
Each value is evaluated when the record is defined.
(In the future the key will also support any type, and the value will only be
evaluated when the key is accessed, much like functions.)

**Examples:** `{ name: "Omar Little"  age: (- 2008 1966)  alive: False }`

#### Functions

Functions take an input value and produce an output value. This happens by
evaluating the function’s body expression with the given arguments added to
the environment. The environment is captured when the function is defined,
making functions closures.

Typically, the input value is a list of arguments. For example, the expression
`(inc 54)` calls the function `inc` with a list of one value, the number `54`,
returning `55`.

**Examples:** `list`, `not`, `apply`, `map`, `sqrt`, `println`

#### Primitives

Functions written in an external language other than eevo, such as the ones
built-in to the language written in C.
They behave like normal functions but are opaque, their implementation cannot
be inspected within the language.

See built-ins section for more examples.

**Examples:** `fst`, `write`, `+`

#### Macros

Functions that operate at the syntax level, accepting code as input and return
code as output.
Unlike functions they receive their arguments unevaluated, and return code that
gets evaluated in the context the macro is called in.

This enables eevo to extend its own syntax, eliminate repetition, construct
custom languages for specific tasks, or directly transform source code.

**Examples:** `if`, `and`, `or`, `let`

#### Forms

Macros written in an external language; like macros, their arguments
are not necessarily evaluated, and like primitives, their internals cannot be
inspected.

**Examples:** `cond`, `def`, `quote`

## Procedures

Procedures are the verbs of eevo, they describe how expressions change.
Procedures can be either functions, macros, primitives, or forms.

*Convention*: Procedures which return a boolean type should end with `?` (**eg**
`pair?`, `integer?`, `even?`), procedures with side-effects should end with `!`
(**eg** `cd!`, `exit!`).

The following are core procedures implemented in C and provided by default in
the base environment.

### fst

Returns first element of given list.

### rst

Return rest of the given list, either just the second element if it is a pair,
or another list with the first element removed.

### Pair

Creates a new pair with the two given arguments, first one as the fst, second
as the rst. Can be chained together to create a list if ending with `Nil`.

See also Pair and List types.

### quote

Prevents evaluation of an expression. Used to convert code to data, or create
lists and symbols without evaluation.

Equivalent to single quote as prefix.

```
(quote x) ; returns the symbol x
'(a b c)  ; returns a list of 3 symbols, symbols do not get evaulated
'(* 3 4)  ; returns a list of 3 expressions, instead of 12
```

### eval

Evaluates the expression given. Can be dangerous to use as arbitrary code could
be executed if the input is not from a trusted source. In general `apply`
should be used when possible.

### =

Tests if values are all equal. Returns `Nil` if any are not, and `True` if they
are.

### cond

Evaluates each expression if the given condition corresponding to it is true.
Runs through all arguments, each is a list with the first element as the
condition which needs to evaluate to `True`, and the rest of the list is
the body to be run if and only if the condition is met. Used for if/elseif/else
statements found in C-like languages.

Also see `if`,`when`,`unless`,`switch`.

### typeof

Returns a string stating the given argument's type.

### Func

Creates anonymous function, first argument is a list of symbols for the names
of the new function arguments. Rest of the arguments to Func is the body of
code to be run when the function is called.

Also see `def`.

### Macro

Transformers which operate on syntax expressions, and return syntax. Similar to
Func, Macro creates anonymous macro with first argument as macro's argument
list and rest as macro's body. Unlike functions macros do not evaluate their
arguments when called, allowing the expressions to be transformed by the macro,
returning a new expression to be evaluated at run time.

Also see `defmacro`.

### def

Create new symbol with the name of the first argument, and the value of the
second. If the name given is a list use the first element of this list as a new
functions name and rest of list as its arguments. If only one argument is given
define a self evaluating symbol.

### undefine!

Remove symbol from environment. Errors if symbol is not defined before.

### defined?

Return boolean on if symbol is defined in the environment.

### load

Loads the library given as a string.

### error

Throw error, print message given by second argument string with the first
argument being a symbol of the function throwing the error.

## Differences From Lisp

### No Mutation

A value can not be changed after it has been defined (since there is no `set!`
function).
The same symbol already defined can be redefined so that a new value shadows
the previous one, but the value itself is not modified.
A variable shadowed in a new scope returns to its original value when that
scope exits.
For example, a function can not change a variable outside its scope:

```
def x 64
def x 54
def add(a)
  def x 12
  + x a

add 1   ; = 13
x       ; = 54 (not 64 or 12)
```

### Less Parenthesis

In eevo parenthesis are implied around every new line, and a line indented more
than the previous one is a sub-expression of it. A line with only one
expression stays that expression, not a list of length one. For example:

```
a b c
  d e
    f
  g h i
```

Becomes:

```
(a b c
   (d e
      f)
   (g h i))
```

### Simpler

eevo only has one builtin equality primitive, `=`, which tests numbers, text,
lists, and objects which occupy the same space in memory, such as primitives.

eevo is single value named, so procedures share the same namespace as
variables. This way functions are full first class citizens. It removes the
need for common lisp's `defunc` vs `defvar`, `let` vs `flet`, and redundant
syntax for getting the function from a symbol.

Symbols are case sensitive, unlike many other older lisps, in order to better
interface with modern languages.

### Read-Print Symmetry

By default eevo's output is valid eevo code, fully equivalent to the evaluated
input.
Lists and symbols are quoted (`(list 1 2 3) => '(1 2 3)`), errors are comments.
The only exception is anonymous functions/macros which will be supported soon.
To print a value as valid eevo code use `display` and `displayln`, to get a
plain output use `print` and `println`.

### Legacy Free

The traditional `car`/`cdr` have little meaning beyond their historical use, so instead replace
them with `fst`/`rst` to get the first expression in a list and the rest of the list respectively.
Compositions can be shorted just like with cxr: `fst(fst(x))` is the same as `ffst(x)`,
`rst(fst(rst(x)))` is `rfrst(x)`, etc.
The combination of `fst(rst(x))` is given the name `snd` since it is very common to get the
second expression of a list, and to avoid confusing `frst` as first.
The function to construct a new pair is changed from `cons` to `Pair` to stay consistent with
other ways to create new types.

Likewise, `lambda` is renamed to `Func`, since function has a clear well known meaning,
opposed to lambda which has meaning only from convention.

## See Also

eevo(1)

See project at <https://edryd.org/projects/eevo>

View source code at <https://git.edryd.org/eevo>

## Author

Edryd van Bruggen <ed@edryd.org>

## License

zlib License
