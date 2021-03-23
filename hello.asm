; ------------------------------------------------------------------------------
; Writes "Hello, World" to the console and then exits.
; To assemble and run:
;
;     $ nasm -felf64 -gdwarf hello.asm
;     $ ld -o hello hello.o
;     $ ./hello
; ------------------------------------------------------------------------------

; ------------------------------------------------------------------------------
        global    _start
; ------------------------------------------------------------------------------

; ------------------------------------------------------------------------------
        section   .text	              ; The start of the code portion of the program.
	
_start:				      ; Labels the entry point to the program.
	mov       rax, 1              ; rax gets the system call code for "write".
        mov       rdi, 1              ; rdi gets the file handle for stdout (console).
        mov       rsi, greetings      ; rsi gets the address of the string below.
        mov       rdx, 25             ; rdx gets the number of bytes to write.
        syscall                       ; Call kernel, triggering the write.  The
	                              ; registers carry the arguments.
	
        mov       rax, 60             ; rax gets the system call code for "exit"
        sub       rdi, rdi            ; rdi = rdi - rdi = 0
				      ; or the exit code for "normal process end".
        syscall			      ; Call kernel, ending the process.
; ------------------------------------------------------------------------------

; ------------------------------------------------------------------------------
        section   .data		      ; The start of the data portion of the program.

greetings:			      ; Labels the string of bytes to be written.
	db        "Finally, got it working!", 10  ; The message, with the byte values for "newline" appended.
; ------------------------------------------------------------------------------
