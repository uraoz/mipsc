.data
stack: .space 4096
.text
.globl main
.globl __start
__start:
	la $t0, stack
	addi $sp, $t0, 4096
	move $fp, $sp
	jal main
	nop
	move $a0, $v0
	li $v0, 1
	syscall
main:
	addiu $sp, $sp, -8
	sw $ra, 4($sp)
	sw $fp, 0($sp)
	move $fp, $sp
	li $t0, 42
	addi $sp,$sp, -4
	sw $t0, 0($sp)
	lw $v0, 0($sp)
	addi $sp, $sp, 4
	move $sp, $fp
	lw $ra, 4($sp)
	lw $fp, 0($sp)
	addiu $sp, $sp, 8
	jr $ra
	nop
