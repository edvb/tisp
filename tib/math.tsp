;;; Constants

def pi (* 4 arctan(1.))
def tau (* 2 pi)
def e exp(1.)

;;; Tests

def positive?(x) (> x 0)
def negative?(x) (< x 0)
def zero?(x)     (= x 0)
def even?(x)     (= mod(x 2) 0)
def odd?(x)      (= mod(x 2) 1)

;;; Functions

def (/= . x) (not apply(= x))
def (*^ x p) (* x (^ 10 p))
def inc(x) (+ x 1)
def dec(x) (- x 1)
def truncate(x) (* floor(abs(x)) sgn(x))
def sqr(x) (* x x)
def cube(x) (* x (* x x))
def root(b p) (^ b (/ p))
def sqrt(x) root(x 2)
def cbrt(x) root(x 3)
def logb(b x) (/ log(x) log(b))
def log10(x) logb(10. x)
def half(x) (/ x 2)
def double(x) (* x 2)

def factorial(n)
  "Product of all positive integers less than or equal to n"
  recur loop (n: n  res: 1)
    if (<= n 1)
      res
      loop (- n 1) (* n res)

def gcd(x y)
  "Largest positive integer that divides both x and y"
  if (= y 0)
    x
    gcd(y mod(x y))

; TODO mv to units tib
def deg(x)
  "Convert x radians to degrees"
  * x (/ 180. pi)
def rad(x)
  "Convert x degrees to radians"
  * x (/ pi 180.)

;;; Trig

def csc(x)     (/ sin(x))
def arccsc(x)  (/ arcsin(x))
def csch(x)    (/ sinh(x))
def arccsch(x) (/ arcsinh(x))
def sec(x)     (/ cos(x))
def arcsec(x)  (/ arccos(x))
def sech(x)    (/ cosh(x))
def arcsech(x) (/ arccosh(x))
def cot(x)     (/ tan(x))
def arccot(x)  (/ arctan(x))
def coth(x)    (/ tanh(x))
def arccoth(x) (/ arctanh(x))

def abs(x)
  "Absolute value: non-negative value of x"
  if (>= x 0)
    x
    - x
def sgn(x)
  "Sign of x: either +1, -1, or 0"
  if (= x 0)
    x
    / abs(x) x
def max(a b) ; TODO many args
  "Maximum of a and b\n
  > max 1 5      ; 5
  > max 4/5 -3   ; 4/5"
  if (> a b) a b
def (min a b)
  "Minimum of a and b\n
  > min 42 54     ; 42
  > min .4 16/3   ; .4"
  if (< a b) a b
def clamp(n lower upper)
  "Limit n between lower and upper bound\n
  > clamp 1 0 10   ; 1
  > clamp -3 2 4   ; 2"
  min max(n lower) upper

;;; List Functions

def dot(v w)
  "Dot product of lists v and w
   Sum of the products of elements of both lists"
  if v
    + (* car(v) car(w))
      dot cdr(v) cdr(w)
    0
def norm(v) ; TODO combine with abs
  "Normalize list"
  sqrt dot(v v)

def mean(lst)
  "Arithmetic mean of list\n
  > mean [1 2 3 4 5]   ; 3
  > mean [5 6 7 8]     ; 13/2"
  recur loop (lst: lst  len: 0  avg: 0)
    if pair?(lst)
      loop cdr(lst) inc(len) (+ avg car(lst))
      / avg len

def range(start end)
  "List ranging from start to end, inclusive\n
  > range(1 5)   ; [1 2 3 4 5]
  > range -4 0   ; [-4 -3 -2 -1 0]"
  recur f (n: end  res: Nil)
    if (>= n start)
      f dec(n) (cons n res)
      res
