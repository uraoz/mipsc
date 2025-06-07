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
	addiu $sp, $sp, -40
	sw $ra, 36($sp)
	sw $s8, 32($sp)
	addiu $s8, $sp, 32
	li $t0, 0
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t0, 0($sp)
	addiu $sp, $sp, 4
	addiu $t0, $s8, -28
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	li $t0, 0
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t1, 0($sp)
	addiu $sp, $sp, 4
	lw $t0, 0($sp)
	addiu $sp, $sp, 4
	li $t2, 4
	mul $t1, $t1, $t2
	add $t0, $t0, $t1
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	li $t0, 10
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
	addiu $t0, $s8, -28
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	li $t0, 1
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t1, 0($sp)
	addiu $sp, $sp, 4
	lw $t0, 0($sp)
	addiu $sp, $sp, 4
	li $t2, 4
	mul $t1, $t1, $t2
	add $t0, $t0, $t1
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	li $t0, 20
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
	addiu $t0, $s8, -28
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	li $t0, 2
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t1, 0($sp)
	addiu $sp, $sp, 4
	lw $t0, 0($sp)
	addiu $sp, $sp, 4
	li $t2, 4
	mul $t1, $t1, $t2
	add $t0, $t0, $t1
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	li $t0, 30
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
	addiu $t0, $s8, -28
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	li $t0, 0
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t1, 0($sp)
	addiu $sp, $sp, 4
	lw $t0, 0($sp)
	addiu $sp, $sp, 4
	li $t2, 4
	mul $t1, $t1, $t2
	add $t0, $t0, $t1
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t0, 0($sp)
	lw $t0, 0($t0)
	sw $t0, 0($sp)
	addiu $t0, $s8, -28
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	li $t0, 2
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t1, 0($sp)
	addiu $sp, $sp, 4
	lw $t0, 0($sp)
	addiu $sp, $sp, 4
	li $t2, 4
	mul $t1, $t1, $t2
	add $t0, $t0, $t1
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t0, 0($sp)
	lw $t0, 0($t0)
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
	lw $ra, 4($s8)
	lw $s8, 0($s8)
	addiu $sp, $sp, 40
	jr $ra
	nop
