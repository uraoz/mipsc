#include "mipsc.h"

void gen(Node* node) {
	if (node->kind == ND_NUM) {
		printf("	li $t0, %d\n", node->val);
		printf("	addi $sp,$sp, -4\n");
		printf("	sw $t0, 0($sp)\n");
		return;
	}
	gen(node->lhs);
	gen(node->rhs);
	printf("	lw $t1,0($sp)\n");
	printf("	addi $sp, $sp, 4\n");
	printf("	lw $t0, 0($sp)\n");
	printf("	addi $sp, $sp, 4\n");
	switch (node->kind) {
	case ND_ADD:
		printf("	add $t0, $t1, $t0\n");
		break;
	case ND_SUB:
		printf("	sub $t0, $t0, $t1\n");
		break;
	case ND_MUL:
		printf("	mul $t0, $t0 ,$t1\n");
		break;
	case ND_DIV:
		printf("	div $t0, $t1\n");
		printf("	mflo $t0\n");
		break;
	case ND_EQ:
		printf("	seq $t0, $t0, $t1\n");
		break;
	case ND_NE:
		printf("	sne $t0, $t0, $t1\n");
		break;
	case ND_LT:
		printf("	slt $t0, $t0, $t1\n");
		break;
	case ND_LE:
		printf("	sle $t0, $t0, $t1\n");
		break;
	default:
		error("ïsê≥Ç»ââéZéqÇ≈Ç∑");
	}

	printf("	addi $sp,$sp, -4\n");
	printf("	sw $t0, 0($sp)\n");
}