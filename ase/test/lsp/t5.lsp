; Compute the N'th Fibonacci number.
(defun fibonacci (N)
  (if (or (zerop N) (= N 1))
      1
    (+ (fibonacci (- N 1)) (fibonacci (- N 2)))))

(fibonacci 5)
