.data
stack: .space 4096
.text
.globl main
.globl __start
__start:
	addi $fp, $sp, 0
	addi $sp, $sp, -4096
ret42:
	addiu $sp, $sp, -8
	sw $ra, 4($sp)
	sw $fp, 0($sp)
	addiu $fp, $sp, 8
	li $t0, 42
	addiu $sp, $sp, -4
	sw $t0, 0($sp)
	lw $v0, 0($sp)
	addiu $sp, $sp, 4
	move $sp, $fp
	lw $fp, -8($fp)
	lw $ra, -4($fp)
	jr $ra
	nop
main:
	addiu $sp, $sp, -8
	sw $ra, 4($sp)
	sw $fp, 0($sp)
	addiu $fp, $sp, 8
	jal ret42
	nop
	addiu $sp, $sp, -4
	sw $v0, 0($sp)
	lw $v0, 0($sp)
	addiu $sp, $sp, 4
	move $a0, $v0
	li $v0, 4001
	syscall
