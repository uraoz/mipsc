.data
stack: .space 4096
.text
.globl main
.globl __start
__start:
	addi $fp, $sp, 0
	addi $sp, $sp, -4096
	li $t0, 42
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
