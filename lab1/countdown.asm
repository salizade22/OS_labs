; ------------------------------------------------------------------------------
; Counts down from 9 to 0, printing each.
; To assemble and run:
;
;     $ nasm -felf64 -gdwarf countdown.asm
;     $ ld -o countdown countdown.o
;     $ ./countdown	
; ------------------------------------------------------------------------------

; ------------------------------------------------------------------------------
        global    _start
; ------------------------------------------------------------------------------

; ------------------------------------------------------------------------------
        section   .text

_start:

        ;; YOUR CODE GOES HERE.
        ;; Make the program print, one line at a time: 9, 8, 7, ..., 1, 0.
	cmp byte[digit_str], '0' ;Compare the current digit and 0
	jl  ret			 ; if the current digit is less than 0, so we finish printing
	mov rax,1
	mov rdi,1
	mov rsi, digit_str
	mov rdx,2
	syscall
	sub byte[digit_str], 1
	jmp _start



	
	;; This code ends the program.
ret:	
	mov       rax, 60                 ; system call for exit
        xor       rdi, rdi                ; exit code 0
        syscall                           ; invoke operating system to exit
; ------------------------------------------------------------------------------

; ------------------------------------------------------------------------------
        section   .data
digit_str:
	db        "9", 10                 ; The integer 10 is a newline char
; ------------------------------------------------------------------------------
