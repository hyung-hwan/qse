;Compute the factorial of N.
(defun factorial (N)
	(if (= N 1)
		1
		(* N (factorial (- N 1)))))

(factorial 10)
