.text
.globl main
.globl __start
__start:
main:
	li $a0,42
	li $v0,4001
	syscall