.data
stack: .space 4096
.text
.globl main
.globl __start
__start:
	addi $fp, $sp, 0
	addi $sp, $sp, -4096
ret42:
	addiu $sp, $sp, -12
	sw $ra, 8($sp)
	sw $fp, 4($sp)
	move $fp, $sp
	li $t0, 42
	addi $sp,$sp, -4
	sw $t0, 0($sp)
	lw $v0, 0($sp)
	addi $sp, $sp, 4
	move $sp, $fp
	lw $ra, 8($sp)
	lw $fp, 4($sp)
	addiu $sp, $sp, 12
	jr $ra
	nop
.L_func_end_ret42:
	li $v0, 0
	move $sp, $fp
	lw $ra, 8($sp)
	lw $fp, 4($sp)
	addiu $sp, $sp, 12
	jr $ra
	nop
main:
	addiu $sp, $sp, -12
	sw $ra, 8($sp)
	sw $fp, 4($sp)
	move $fp, $sp
	jal ret42
	nop
	addi $sp, $sp, -4
	sw $v0, 0($sp)
	lw $v0, 0($sp)
	addi $sp, $sp, 4
	move $a0, $v0
	li $v0, 4001
	syscall
.L_func_end_main:
	li $a0, 0
	li $v0, 4001
	syscall
