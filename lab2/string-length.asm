;;; ----------------------------------------------------------------------------------------
;;; Given a string to print (see data section), determine its length, write it to
;;; the console, and then exit.
;;; ----------------------------------------------------------------------------------------

;;; ----------------------------------------------------------------------------------------
	global  string_length
;;; ----------------------------------------------------------------------------------------

;;; ----------------------------------------------------------------------------------------
        section	.text	              ; The start of the code portion of the program.

;;; COMPLETE THE string_length FUNCTION.
string_length:
	mov rax, 0

start:
	cmp byte[rdi], 0
	jle stop
	inc rdi
	inc rax
	jmp start

stop:
	sub rdi, rax
	ret

;;; ----------------------------------------------------------------------------------------
