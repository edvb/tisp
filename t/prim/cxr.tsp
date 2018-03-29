(car (cons 1 2))
(cdr (cons 1 2))

(car (quote (1 2 3 4)))
(car (cdr (quote (1 2 3 4))))
(car (cdr (cdr (quote (1 2 3 4)))))
(car (cdr (cdr (cdr (quote (1 2 3 4))))))

(cdr (quote (1 2 3 4)))
(cdr (cdr (quote (1 2 3 4))))

(car (cons 1 (cons 2 3)))
(cdr (cons 1 (cons 2 3)))
(cdr (cdr (cons 1 (cons 2 3))))
