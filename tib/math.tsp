;;; Constants

(def pi (* 4 (arctan 1.)))
(def tau (* 2 pi))
(def e (exp 1.))

;;; Functions

(def (/= . x) (not (apply = x)))
(def (*^ x p) (* x (^ 10 p)))
(def (inc x) (+ x 1))
(def (dec x) (- x 1))
(def (truncate x) (* (floor (abs x)) (sgn x)))
(def (sqr x) (* x x))
(def (cube x) (* x (* x x)))
(def (root b p) (^ b (/ p)))
(def (sqrt x) (root x 2))
(def (cbrt x) (root x 3))
(def (logb b x) (/ (log x) (log b)))
(def (log10 x) (logb 10. x))
(def half(x) (/ x 2))
(def double(x) (* x 2))

(def factorial(n)
  (recur loop (n: n  res: 1)
    (if (<= n 1)
      res
      (loop (- n 1) (* n res)))))

(def gcd(x y)
  (if (= y 0)
    x
    gcd(y mod(x y))))

; TODO mv to units tib
(def (deg x)
  "Convert x radians to degrees"
  (* x (/ 180. pi)))
(def (rad x)
  "Convert x degrees to radians"
  (* x (/ pi 180.)))

;;; Trig

(def (csc x)     (/ (sin x)))
(def (arccsc x)  (/ (arcsin x)))
(def (csch x)    (/ (sinh x)))
(def (arccsch x) (/ (arcsinh x)))
(def (sec x)     (/ (cos x)))
(def (arcsec x)  (/ (arccos x)))
(def (sech x)    (/ (cosh x)))
(def (arcsech x) (/ (arccosh x)))
(def (cot x)     (/ (tan x)))
(def (arccot x)  (/ (arctan x)))
(def (coth x)    (/ (tanh x)))
(def (arccoth x) (/ (arctanh x)))

(def (abs x) (if (>= x 0) x (- x)))
(def (sgn x) (if (= x 0) x (/ (abs x) x)))
; TODO many args
(def (max a b) (if (> a b) a b))
(def (min a b) (if (< a b) a b))
(def clamp(n lower upper) min(max(n lower) upper))

;;; Tests

(def (positive? x) (> x 0))
(def (negative? x) (< x 0))
(def (zero? x) (= x 0))
(def (even? x) (= (mod x 2) 0))
(def (odd? x)  (= (mod x 2) 1))

;;; List Functions

(def (dot v w)
  (if v
    (+ (* (car v) (car w))
       (dot (cdr v) (cdr w)))
    0))
(def (norm v) (sqrt (dot v v)))

(def (mean lst)
  (recur loop (lst: lst len: 0 avg: 0)
    (if pair?(lst)
      loop(cdr(lst) inc(len) (+ avg car(lst)))
      (/ avg len))))

(def range(start end)
  (recur f (n: end res: Nil)
    (if (>= n start)
      (f dec(n) (cons n res))
      res)))
