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
	addiu $sp, $sp, -8
	sw $ra, 4($sp)
	sw $s8, 0($sp)
	addiu $s8, $sp, 0
	li $t0, 0
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	li $t0, 5
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t1, 0($sp)
	addiu $sp, $sp, 4
	lw $t0, 0($sp)
	addiu $sp, $sp, 4
	sub $t0, $t0, $t1
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $v0, 0($sp)
	addiu $sp, $sp, 4
	lw $ra, 4($s8)
	lw $s8, 0($s8)
	addiu $sp, $sp, 8
	jr $ra
	nop
