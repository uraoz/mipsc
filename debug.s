.data
stack: .space 4096
.text
.globl main
.globl __start
__start:
	addi $s8, $sp, 0
	addi $sp, $sp, -4096
	jal main
	nop
	move $a0, $v0
	li $v0, 4001
	syscall
main:
	addiu $sp, $sp, -24
	sw $ra, 20($sp)
	sw $s8, 16($sp)
	addiu $s8, $sp, 16
	addiu $t0, $s8, -12
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	li $t0, 0
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t1, 0($sp)
	addiu $sp, $sp, 4
	lw $t0, 0($sp)
	addiu $sp, $sp, 4
	sw $t1, 0($t0)
	addiu $sp, $sp, -4
	sw $t1, 0($sp)
	lw $t0, 0($sp)
	addiu $sp, $sp, 4
	li $t0, 0
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t0, 0($sp)
	addiu $sp, $sp, 4
	addiu $t0, $s8, -16
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	li $t0, 1
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t1, 0($sp)
	addiu $sp, $sp, 4
	lw $t0, 0($sp)
	addiu $sp, $sp, 4
	sw $t1, 0($t0)
	addiu $sp, $sp, -4
	sw $t1, 0($sp)
	lw $t0, 0($sp)
	addiu $sp, $sp, 4
.L_begin_0:
	lw $t0, -16($s8)
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	li $t0, 4
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t1, 0($sp)
	addiu $sp, $sp, 4
	lw $t0, 0($sp)
	addiu $sp, $sp, 4
	sle $t0, $t0, $t1
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t0, 0($sp)
	addiu $sp, $sp, 4
	beq $t0, $zero, .L_end_0
	addiu $t0, $s8, -12
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	addiu $t0, $s8, -12
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t0, 0($sp)
	lw $t0, 0($t0)
	sw $t0, 0($sp)
	lw $t0, -16($s8)
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t1, 0($sp)
	addiu $sp, $sp, 4
	lw $t0, 0($sp)
	addiu $sp, $sp, 4
	add $t1, $t0, $t1
	lw $t0, 0($sp)
	addiu $sp, $sp, 4
	sw $t1, 0($t0)
	addiu $sp, $sp, -4
	sw $t1, 0($sp)
	addiu $t0, $s8, -16
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t0, 0($sp)
	lw $t1, 0($t0)
	addiu $t2, $t1, 1
	sw $t2, 0($t0)
	sw $t1, 0($sp)
	lw $t0, 0($sp)
	addiu $sp, $sp, 4
	j .L_begin_0
.L_end_0:
	lw $t0, -12($s8)
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $v0, 0($sp)
	addiu $sp, $sp, 4
	lw $ra, 4($s8)
	lw $s8, 0($s8)
	addiu $sp, $sp, 24
	jr $ra
	nop
