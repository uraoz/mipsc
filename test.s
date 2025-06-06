.data
stack: .space 4096
.text
.globl main
.globl __start
__start:
main:
	addi $fp, $sp, 0
	addi $sp, $sp, -4096
	addi $t0, $fp, -8
	addi $sp, $sp, -4
	sw $t0, 0($sp)
	li $t0, 5
	addi $sp,$sp, -4
	sw $t0, 0($sp)
	lw $t1, 0($sp)
	addi $sp, $sp, 4
	lw $t0, 0($sp)
	addi $sp, $sp, 4
	sw $t1, 0($t0)
	addi $sp, $sp, -4
	sw $t1, 0($sp)
	lw $t1, 0($sp)
	addi $sp, $sp, 4
	addi $t0, $fp, -8
	addi $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t0, 0($sp)
	addi $sp, $sp, 4
	lw $t0, 0($t0)
	addi $sp, $sp, -4
	sw $t0, 0($sp)
	lw $t1, 0($sp)
	addi $sp, $sp, 4
	addi $a0, $t1, 0
	li $v0, 4001
	syscall
	lw $t1, 0($sp)
	addi $sp, $sp, 4
	addi $a0, $t1, 0
	li $v0,4001
	syscall
