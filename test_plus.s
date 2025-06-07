.data
stack: .space 4096
.text
.globl main
.globl __start
__start:
	addi $fp, $sp, 0
	addi $sp, $sp, -4096
	jal main
	nop
	move $a0, $v0
	li $v0, 4001
	syscall
plus:
	addiu $sp, $sp, -8
	sw $ra, 4($sp)
	sw $fp, 0($sp)
	addiu $fp, $sp, 8
	sw $a0, 0($fp)
	sw $a1, 4($fp)
	lw $t0, 0($fp)
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t0, 4($fp)
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t1, 0($sp)
	addiu $sp, $sp, 4
	lw $t0, 0($sp)
	addiu $sp, $sp, 4
	add $t0, $t0, $t1
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $v0, 0($sp)
	addiu $sp, $sp, 4
	move $sp, $fp
	lw $fp, -8($sp)
	lw $ra, -4($sp)
	addiu $sp, $sp, 8
	jr $ra
	nop
main:
	addiu $sp, $sp, -8
	sw $ra, 4($sp)
	sw $fp, 0($sp)
	addiu $fp, $sp, 8
	li $t0, 3
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	li $t0, 4
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $a1, 0($sp)
	addiu $sp, $sp, 4
	lw $a0, 0($sp)
	addiu $sp, $sp, 4
	jal plus
	nop
	addiu $sp, $sp, -4
	sw $v0, 0($sp)
	lw $v0, 0($sp)
	addiu $sp, $sp, 4
	move $a0, $v0
	li $v0, 4001
	syscall
