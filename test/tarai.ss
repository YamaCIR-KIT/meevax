(define tarai
  (lambda (x y z)
    (if (not (< y x)) y
        (tarai (tarai (- x 1) y z)
               (tarai (- y 1) z x)
               (tarai (- z 1) x y)))))

(let ((result (tarai 12 6 0)))
  (display "result")
  (newline))