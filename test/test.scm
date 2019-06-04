(load "../setup.scm")

(define test
  (macro (expression expects)
   `(let ((result ,expression))
      (if (not (equal? result ',expects))
          (begin (display "; test          ; expected ") ; TODO SUPPORT TAB
                 (display ',expects)
                 (display " as result of ")
                 (display ',expression)
                 (display ", but got ")
                 (display result)
                 (display ".\n")
                 (emergency-exit))
          result))))

; ------------------------------------------------------------------------------
;   4.1.1 Variable References
; ------------------------------------------------------------------------------

(define x 28)

(test x 28)


; ------------------------------------------------------------------------------
;   4.1.2 Literal Expressions
; ------------------------------------------------------------------------------

(test
  (quote a)
  a)

; (test
;   (quote #(a b c)) ; unimplemented
;   #(a b c))

(test
  (quote (+ 1 2))
  (+ 1 2))

(test 'a a)

; (test
;  '#(a b c)
;   #(a b c)) ; unimplemented

(test
 '()
  ())

(test
 '(+ 1 2)
  (+ 1 2))

(test
 '(quote a)
  (quote a))

(test
  ''a
  (quote a))

(test '145932 145932)
(test  145932 145932)

; (test '"abc" "abc")
; (test  "abc" "abc") ; マクロ中で文字列の挙動がおかしいバグあり

; (test '# #)
; (test  # #) ; unimplemented

; (test '#(a 10) #(a 10))
; (test  #(a 10) #(a 10)) ; unimplemented

; (test '#u8(64 65) #u8(64 65))
; (test  #u8(64 65) #u8(64 65)) ; unimplemented

(test '#t #t)
(test  #t #t)


; ------------------------------------------------------------------------------
;   4.1.3 Procedure Calls
; ------------------------------------------------------------------------------

(test (+ 3 4) 7)

(test
  ((if #false + *) 3 4)
  12)


; ------------------------------------------------------------------------------
;   4.1.4 Procedures
; ------------------------------------------------------------------------------

; (lambda (x) (+ x x)) ; untestable output

(test
  ((lambda (x) (+ x x)) 4)
  8)

(define reverse-subtract
  (lambda (x y)
    (- y x)))

(test
  (reverse-subtract 7 10)
  3)

(define add4
  (let ((x 4))
    (lambda (y) (+ x y))))

(test
  (add4 6)
  10)

(test
  ((lambda x x) 3 4 5 6)
  (3 4 5 6))

(test
  ((lambda (x y . z) z) 3 4 5 6)
  (5 6))


; ------------------------------------------------------------------------------
;   6.1 Equivalence Predicates
; ------------------------------------------------------------------------------

(test
  (eqv? 'a
        'a)
  #true)

(test
  (eqv? 'a
        'b)
  #false)

(test
  (eqv? '()
        '())
  #true)

(test
  (eqv? 2
        2)
  #true)

; (test ; unimplemented
;   (eqv? 2 2.0)
;   #false)

(test
  (eqv? 100000000
        100000000)
  #true)

; (test ; unimplemented
;   (eqv? 0.0 +nan.0)
;   #false)

(test
  (eqv? (cons 1 2)
        (cons 1 2))
  #false)

(test
  (eqv? (lambda () 1)
        (lambda () 2))
  #false)

(test
  (let ((p (lambda (x) x)))
    (eqv? p p))
  #true)

(test
  (eqv? #false 'nil)
  #false)


; (eqv? "" ""); unspecified

; (eqv? '#() '#()); #unspecified

; (eqv? (lambda (x) x)
;       (lambda (x) x)); #unspecified

; (eqv? (lambda (x) x)
;       (lambda (x) y)); #unspecified

; (eqv? 1.0e0 1.0f0); #unspecified

; (eqv? +nan.0 +nan.0); #unspecified


(define generate-counter
  (lambda ()
    (let ((n 0))
      (lambda ()
        (set! n (+ n 1))
        n))))

(test
  (let ((g (generate-counter)))
    (eqv? g g))
  #true)

(test
  (eqv? (generate-counter)
        (generate-counter))
  #false)

(define generate-loser
  (lambda ()
    (let ((n 0))
      (lambda ()
        (set! n (+ n 1))
        27))))

(test
  (let ((g (generate-loser)))
    (eqv? g g))
  #true)

; (eqv? (generate-loser) (generate-loser)); #unspecified

; (letrec ((f (lambda () (if (eqv? f g) 'both 'f)))
;          (g (lambda () (if (eqv? f g) 'both 'g))))
;   (eqv? f g)); #unspecified

; (test; #unimplemented
;   (letrec ((f (lambda () (if (eqv? f g) ’f ’both)))
;            (g (lambda () (if (eqv? f g) ’g ’both))))
;     (eqv? f g))
;   #false)

; (eqv? '(a) '(a)); #unspecified
; (eqv? "a" "a"); #unspecified
; (eqv? '(b) (cdr '(a b))); #unspecified

(test
  (let ((x '(a)))
    (eqv? x x))
  #true)

(test
  (eq? 'a
       'a)
  #true)

; (eq? '(a) '(a)); #unspecified

(test
  (eq? (list 'a)
       (list 'a))
  #false)

; (eq? "a" "a"); #unspecified
; (eq? "" ""); #unspecified

(test
  (eq? '()
       '())
  #true)

; (eq? 2 2); #unspecified
; (eq? #\A #\A); #unspecified

(test
  (eq? car car)
  #true)

; (let ((n (+ 2 3)))
;   (eq? n n)); #unspecified

(test
  (let ((x '(a)))
    (eq? x x))
  #true)

; (test
;   (let ((x '#())) ; #unimplemented
;     (eq? x x))
;   #true)

(test
  (let ((p (lambda (x) x)))
    (eq? p p))
  #true)

(test
  (equal? 'a
          'a)
  #true)

(test
  (equal? '(a)
          '(a))
  #true)

(test
  (equal? '(a (b) c)
          '(a (b) c))
  #true)

; (test
;   (equal? "abc"
;           "abc") ; not fully supported yet
;   #true)

(test
  (equal? 2 2)
  #true)

; (equal? (make-vector 5 'a)
;         (make-vector 5 'a)); #true
;
; (equal? '#1=(a b . #1#)
;         '#2=(a b a b . #2#)); #true
;
; (equal? (lambda (x) x)
;         (lambda (y) y)); #unspecified



; (define x 42)
; x
; (set! x 100)
; x
;
; (define y 'hoge)
; y
; (set! y 100)
; y
;
; (define accumulator
;   (lambda (n)
;     (lambda ()
;       (set! n (+ n 1)))))
;
; (define acc (accumulator x))
; (acc)
; (acc)
; (acc)
;
; (or)
; (or #true)
; (or #false)
; (or (eq? 'a 'a) (eq? 'b 'b))
; (or (eq? 'a 'b) (eq? 'c 'c))
; (or (eq? 'a 'b) (eq? 'c 'd))
;
; (define a '(1 2 3))
;
; `(a b c); => (a b c)
; `(,a b c); => ((1 2 3) b c)
; `(,@a b c); => (1 2 3 b c)
;
; (define map
;   (lambda (callee &list)
;     (if (null? &list)
;        '()
;         (cons (callee (car &list))
;               (map callee (cdr &list))))))
;
; ; (define let
; ;   (macro (bindings . body)
; ;    `((lambda ,(map car bindings) ,@body)
; ;      ,@(map cadr bindings))))
;
; (let ((a 1)
;       (b 2))
;   (+ a b))
;
; (begin (display 1)
;        (display 2)
;        (display 3))

