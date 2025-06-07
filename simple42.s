.data
stack: .space 4096
.text
.globl main
.globl __start
__start:
	la $sp, stack + 4096
	move $fp, $sp
	jal main
	nop
	move $a0, $v0
	li $v0, 4001
	syscall
main:
	addiu $sp, $sp, -16
	sw $ra, 12($sp)
	sw $fp, 8($sp)
	move $fp, $sp
	li $t0, 42
	addi $sp,$sp, -4
	sw $t0, 0($sp)
	lw $v0, 0($sp)
	addi $sp, $sp, 4
	move $a0, $v0
	li $v0, 4001
	syscall
.L_func_end_main:
	li $a0, 0
	li $v0, 4001
	syscall
