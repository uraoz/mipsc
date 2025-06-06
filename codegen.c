#include "mipsc.h"

void gen_lval(Node* node) {
	if (node->kind != ND_LVAR) {
		error("not left node");
	}
	printf("	addi $t0, $fp, -%d\n", node->offset);
	printf("	addi $sp, $sp, -4\n");
	printf("	sw $t0, 0($sp)\n");
}

void gen(Node* node) {
	switch (node->kind) {
	case ND_BLOCK:
		for (int i = 0; node->body[i]; i++) {
			gen(node->body[i]);
			// ブロック内の各文の結果をスタックから除去
			if (node->body[i]->kind != ND_RETURN && node->body[i]->kind != ND_IF && 
			    node->body[i]->kind != ND_WHILE && node->body[i]->kind != ND_FOR && 
			    node->body[i]->kind != ND_BLOCK) {
				printf("	lw $t0, 0($sp)\n");
				printf("	addi $sp, $sp, 4\n");
			}
		}
		return;
	case ND_IF: {
		int seq = label_count++;
		gen(node->cond);
		printf("	lw $t0, 0($sp)\n");
		printf("	addi $sp, $sp, 4\n");
		printf("	beq $t0, $zero, .L_end_%d\n", seq);
		gen(node->then);
		printf(".L_end_%d:\n", seq);
		return;
	}
	case ND_WHILE: {
		int seq = label_count++;
		printf(".L_begin_%d:\n", seq);
		gen(node->cond);
		printf("	lw $t0, 0($sp)\n");
		printf("	addi $sp, $sp, 4\n");
		printf("	beq $t0, $zero, .L_end_%d\n", seq);
		gen(node->then);
		printf("	j .L_begin_%d\n", seq);
		printf(".L_end_%d:\n", seq);
		return;
	}
	case ND_FOR: {
		int seq = label_count++;
		// 初期化
		if (node->init) {
			gen(node->init);
			printf("	lw $t0, 0($sp)\n");
			printf("	addi $sp, $sp, 4\n");
		}
		printf(".L_begin_%d:\n", seq);
		// 条件チェック
		if (node->cond) {
			gen(node->cond);
			printf("	lw $t0, 0($sp)\n");
			printf("	addi $sp, $sp, 4\n");
			printf("	beq $t0, $zero, .L_end_%d\n", seq);
		}
		// ボディ実行
		gen(node->then);
		// インクリメント
		if (node->inc) {
			gen(node->inc);
			printf("	lw $t0, 0($sp)\n");
			printf("	addi $sp, $sp, 4\n");
		}
		printf("	j .L_begin_%d\n", seq);
		printf(".L_end_%d:\n", seq);
		return;
	}
	case ND_RETURN:
		gen(node->lhs);
		printf("	lw $t1, 0($sp)\n");
		printf("	addi $sp, $sp, 4\n");
		printf("	addi $a0, $t1, 0\n");
		printf("	li $v0, 4001\n");
		printf("	syscall\n");
		return;
	case ND_NUM:
		printf("	li $t0, %d\n", node->val);
		printf("	addi $sp,$sp, -4\n");
		printf("	sw $t0, 0($sp)\n");
		return;
	case ND_LVAR:
		gen_lval(node);
		printf("	lw $t0, 0($sp)\n");
		printf("	addi $sp, $sp, 4\n");
		printf("	lw $t0, 0($t0)\n");
		printf("	addi $sp, $sp, -4\n");
		printf("	sw $t0, 0($sp)\n");
		return;
	case ND_ASSIGN:
		gen_lval(node->lhs);
		gen(node->rhs);
		printf("	lw $t1, 0($sp)\n");
		printf("	addi $sp, $sp, 4\n");
		printf("	lw $t0, 0($sp)\n");
		printf("	addi $sp, $sp, 4\n");
		printf("	sw $t1, 0($t0)\n");
		printf("	addi $sp, $sp, -4\n");
		printf("	sw $t1, 0($sp)\n");
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
	}

	printf("	addi $sp,$sp, -4\n");
	printf("	sw $t0, 0($sp)\n");
}