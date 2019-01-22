; _____ _ _                                _ 
; |  ___(_) |__   ___  _ __   __ _  ___ ___(_)
; | |_  | | '_ \ / _ \| '_ \ / _` |/ __/ __| |
; |  _| | | |_) | (_) | | | | (_| | (_| (__| |
; |_|   |_|_.__/ \___/|_| |_|\__,_|\___\___|_|
;                                           
; Fibonacci for SUBLEQ with pseudo instructions: ADD, MOV, HLT
;
; The pseudo instructions can be mapped on multiple SUBLEQ instructions. The
; pseudocode is:
;
;   fib(n):
;       a = 0, b = 1
;       while (n >= 2):
;           tmp2 = a + b
;           a = b
;           b = tmp2
;           n--
;       return b
;
; 2019 Maximilian Strauch

.def n   0xfa 6             ; Input n for fibonacci
.def a   0xfc 0
.def b   0xfd 1
.def out 0x07 0             ; Output register for display

; Temp registeres, needed since the subleq instructions modify the original
; registers (the program might be optimized to use only one tmp register)
.def tmp1 0xfe 0
.def tmp2 0xff 0

; Constant values. Since subleq has no immediate instructions we store
; those values in a register
.def Z   0x00 0
.def one 0xfb 1
.def two 0xf9 2

    mov n, tmp1             ; Copy n to tmp1 to perform calculation on it
    subleq two, tmp1, done  ; If n <= 2, we're done and goto the end
    subleq one, n, loop     ; Start computation with n := n-1

loop:
    mov a, tmp1             ; Save a and ...
    mov b, tmp2             ; ... b to perform the addition on it
    add tmp1, tmp2          ; Save the sum of a and b on tmp2
    mov b, a                ; Set a to the value of b
    mov tmp2, b             ; Set b to tmp2 which is a + b
    subleq one, n, done     ; Decrement n
    subleq Z, Z, loop       ; Check if n > 0 and continue computation

done:
    mov b, out              ; Finished ;-) - output the value on b
    hlt                     ; Halt the CPU