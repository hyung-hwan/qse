(setq x (lambda (x) (car x)))
((lambda (x) (+ x 99)) (x '(10 20 30)))

((lambda (x) ((lambda (y) (+ y 1)) x)) 10)
; lisp....
((lambda (x) ((macro (y) (+ y 1)) x)) 10)




;;;;;;;
(setq init-rand (macro (seed) (lambda () (setq seed (+ seed 1)))))
(setq init-rand (lambda (seed) (lambda () (setq seed (+ seed 1)))))
(setq rand (init-rand 1))
(rand)

