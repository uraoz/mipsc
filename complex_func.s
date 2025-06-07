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
	li $v0, 4001
	syscall
mul:
	addiu $sp, $sp, -16
	sw $ra, 0($sp)
	sw $s8, 4($sp)
	move $s8, $sp
	sw $a0, 8($s8)
	sw $a1, 12($s8)
	lw $t0, 8($s8)
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t0, 12($s8)
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t1, 0($sp)
	addiu $sp, $sp, 4
	lw $t0, 0($sp)
	addiu $sp, $sp, 4
	mul $t0, $t0, $t1
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $v0, 0($sp)
	addiu $sp, $sp, 4
	move $sp, $s8
	lw $ra, 0($sp)
	lw $s8, 4($sp)
	addiu $sp, $sp, 16
	jr $ra
	nop
add:
	addiu $sp, $sp, -16
	sw $ra, 0($sp)
	sw $s8, 4($sp)
	move $s8, $sp
	sw $a0, 8($s8)
	sw $a1, 12($s8)
	lw $t0, 8($s8)
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t0, 12($s8)
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
	move $sp, $s8
	lw $ra, 0($sp)
	lw $s8, 4($sp)
	addiu $sp, $sp, 16
	jr $ra
	nop
main:
	addiu $sp, $sp, -8
	sw $ra, 0($sp)
	sw $s8, 4($sp)
	move $s8, $sp
	li $t0, 2
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	li $t0, 3
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $a0, 4($sp)
	lw $a1, 0($sp)
	addiu $sp, $sp, 8
	jal mul
	nop
	addiu $sp, $sp, -4
	sw $v0, 0($sp)
	li $t0, 4
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	li $t0, 5
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $a0, 4($sp)
	lw $a1, 0($sp)
	addiu $sp, $sp, 8
	jal mul
	nop
	addiu $sp, $sp, -4
	sw $v0, 0($sp)
	lw $a0, 4($sp)
	lw $a1, 0($sp)
	addiu $sp, $sp, 8
	jal add
	nop
	addiu $sp, $sp, -4
	sw $v0, 0($sp)
	lw $v0, 0($sp)
	addiu $sp, $sp, 4
	move $sp, $s8
	lw $ra, 0($sp)
	lw $s8, 4($sp)
	addiu $sp, $sp, 8
	move $a0, $v0
	li $v0, 4001
	syscall
