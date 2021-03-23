;;; ----------------------------------------------------------------------------------------
;;; Calculate an exponent, output the result to the console, and then exit.
;;;
;;; To assemble and run:
;;;
;;;     $ make exp
;;;     $ ./exp
;;; ----------------------------------------------------------------------------------------

;;; ----------------------------------------------------------------------------------------
        global  main
	global  exp
	extern  printf
;;; ----------------------------------------------------------------------------------------

;;; ----------------------------------------------------------------------------------------
        section	.text	              ; The start of the code portion of the program.

main:	

	;; Call exp to calculate x^y.
	mov	rdi, [x]	      ; arg[0] = base
	mov	rsi, [y]	      ; arg[1] = exponent
	sub	rsp, 8		      ; Align rsp to double-world boundary.
	call	exp
	add	rsp, 8		      ; Restore rsp.

	;; Print the result of x^y.
	mov	rdi, int_format_string ; arg[0] = formatting string.
	mov	rsi, rax	       ; arg[1] = x^y result
	mov	rax, 0		       ; arg[2] = 0 (end of arguments marker for varargs)
	sub	rsp, 8		       ; Align rsp.
	call	printf
	add	rsp, 8		       ; Restore rsp.

	mov	rax, 0		       ; Exit status code = "all normal".
	ret			       ; End of old_main.

exp:	
exp_iterative:
	mov	rax, 1
ei_top:
	cmp	rsi, 0
	jle	ei_end
	imul	rax, rdi
	dec	rsi
	jmp	ei_top
ei_end:
	ret
	
exp_recursive:

	;; Base case: y = 0.
	cmp	rsi, 0		       ; y vs. 0
	jg	exp_recursive_case     ; y > 0 => recursive case
	mov	rax, 1		       ; return value = 1
	ret

exp_recursive_case:

	;; Recursive case: y > 1, so x^y = x * x^(y-1)
	dec	rsi		       ; y = y - 1
	sub	rsp, 8		       ; Align rsp.
	call	exp_recursive	       ; Recur, rax = x^(y-1)
	add	rsp, 8		       ; Restore rsp.
	imul	rax, rdi	       ; rax = rax * x
	ret
;;; ----------------------------------------------------------------------------------------

;;; ----------------------------------------------------------------------------------------
        section   .data		         ; The start of the data portion of the program.

x:	dq	2
y:	dq	5
	
int_format_string:	
	db        "%d", 10, 0	         ; Formatting string for an int, newline, null-terminator.
;;; ----------------------------------------------------------------------------------------
