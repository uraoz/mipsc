.data
stack: .space 4096
.text
.globl main
.globl __start
__start:
	addi $fp, $sp, 0
	addi $sp, $sp, -4096
plus:
	addiu $sp, $sp, -12
	sw $ra, 8($sp)
	sw $fp, 4($sp)
	move $fp, $sp
	sw $4, 12($fp)
	sw $5, 16($fp)
	lw $t0, 12($fp)
	addi $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t0, 16($fp)
	addi $sp, $sp, -4
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
	lw $ra, 8($sp)
	lw $fp, 4($sp)
	addiu $sp, $sp, 12
	jr $ra
	nop
.L_func_end_plus:
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
	li $t0, 3
	addi $sp,$sp, -4
	sw $t0, 0($sp)
	li $t0, 4
	addi $sp,$sp, -4
	sw $t0, 0($sp)
	lw $4, 0($sp)
	addi $sp, $sp, 4
	lw $5, 0($sp)
	addi $sp, $sp, 4
	jal plus
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
