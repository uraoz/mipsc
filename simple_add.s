.data
stack: .space 4096
.text
.globl main
.globl __start
__start:
	addi $fp, $sp, 0
	addi $sp, $sp, -4096
main:
	addiu $sp, $sp, -32
	sw $ra, 28($sp)
	sw $fp, 24($sp)
	move $fp, $sp
	li $t0, 3
	addi $sp,$sp, -4
	sw $t0, 0($sp)
	li $t0, 4
	addi $sp,$sp, -4
	sw $t0, 0($sp)
	lw $t1,0($sp)
	addi $sp, $sp, 4
	lw $t0, 0($sp)
	addi $sp, $sp, 4
	add $t0, $t1, $t0
	addi $sp,$sp, -4
	sw $t0, 0($sp)
	lw $v0, 0($sp)
	addi $sp, $sp, 4
	move $sp, $fp
	lw $ra, 28($sp)
	lw $fp, 24($sp)
	addiu $sp, $sp, 32
	move $a0, $v0
	li $v0, 4001
	syscall
.L_func_end_main:
	move $sp, $fp
	lw $ra, 28($sp)
	lw $fp, 24($sp)
	addiu $sp, $sp, 32
	li $a0, 0
	li $v0, 4001
	syscall
