; $Id$
; sample scheme datatype library for RVP

; (define gc-hook (display "gc-hook\n"))
; (verbose 10)

(define (dsl-string->token s)
  (let loop ((tl '()) (sl (string->list s)) (state #f))
    (if (null? sl) (list->string (reverse tl))
      (let ((c (car sl)))
	(case (char->integer c)
	  ((9 10 13 32)  (loop tl (cdr sl) (and state 'between)))
	  (else
	    (loop
	      (case state
		((#f in) (cons c tl))
		((between) (cons c (cons #\space tl))))
	      (cdr sl)
	      'in)))))))

; (dsl-equal? string string string)	
(define (dsl-equal? typ val s)
  (case (string->symbol typ)
    ((string) (string=? val s))
    ((token) (dsl-equal? "string" (dsl-string->token val) (dsl-string->token s)))
    (else #f)))

; (dsl-allows? string '((string . string)*) string)
(define (dsl-allows? typ ps s)
  (case (string->symbol typ)
    ((string)
      (let params ((ps ps))
	(if (pair? ps)
	  (let ((p (car ps)))
	    (case (string->symbol (car p))
	      ((length)
		(and (<= (string-length s) (string->number (cdr p)))
		  (params (cdr ps))))
	      ((pattern)
		(and #t
		  (params (cdr ps))))
	      (else #f)))
	  #t)))
    ((token) (dsl-allows? "string" ps (dsl-string->token s)))
    (else #t)))
