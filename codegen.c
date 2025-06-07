#include "mipsc.h"

void gen_lval(Node* node) {
	if (node->kind != ND_LVAR) {
		error("not left node");
	}
	printf("	addi $t0, $s8, %d\n", node->offset);
	printf("	addi $sp, $sp, -4\n");
	printf("	sw $t0, 0($sp)\n");
}

void gen(Node* node) {
	switch (node->kind) {
	case ND_FUNC: {
		// 現在の関数名を設定
		current_func_name = node->name;
		
		// 関数の開始ラベル
		printf("%s:\n", node->name);
		
		// 対応する関数のローカル変数情報を取得
		Function* func = NULL;
		for (Function* f = functions; f; f = f->next) {
			if (strcmp(f->name, node->name) == 0) {
				func = f;
				break;
			}
		}
		
		// ローカル変数のサイズ計算
		int local_size = 0;
		if (func && func->locals) {
			for (LVar* var = func->locals; var; var = var->next) {
				if (var->offset < 0) {
					local_size = (-var->offset > local_size) ? -var->offset : local_size;
				}
			}
		}
		
		// スタックフレームサイズ計算（8バイトアライメント）
		// 戻りアドレス(4) + フレームポインタ(4) + ローカル変数 + パディング
		int arg_size = node->argc > 0 ? node->argc * 4 : 0;
		int frame_size = 8 + local_size + arg_size;
		if (frame_size % 8 != 0) {
			frame_size = ((frame_size + 7) / 8) * 8;
		}
		current_frame_size = frame_size;
		
		// 統一された関数プロローグ
		printf("	addiu $sp, $sp, -%d\n", frame_size);
		printf("	sw $ra, 0($sp)\n");
		printf("	sw $s8, 4($sp)\n");
		printf("	move $s8, $sp\n");
		
		// 引数をローカル領域に保存
		if (node->argc > 0) {
			for (int i = 0; i < node->argc && i < 4; i++) {
				printf("	sw $a%d, %d($s8)\n", i, 8 + i * 4);
			}
		}
		
		// 関数本体の実行
		bool has_return = false;
		for (int i = 0; node->body[i]; i++) {
			gen(node->body[i]);
			if (node->body[i]->kind == ND_RETURN) {
				has_return = true;
				break;
			}
			// 各文の結果をスタックから除去
			if (node->body[i]->kind != ND_IF && 
			    node->body[i]->kind != ND_WHILE && 
			    node->body[i]->kind != ND_FOR && 
			    node->body[i]->kind != ND_BLOCK) {
				printf("	lw $t0, 0($sp)\n");
				printf("	addiu $sp, $sp, 4\n");
			}
		}
		
		// デフォルトの戻り値とエピローグ
		if (!has_return) {
			printf(".L_func_end_%s:\n", node->name);
			printf("	li $v0, 0\n");
			printf("	move $sp, $s8\n");
			printf("	lw $ra, 0($sp)\n");
			printf("	lw $s8, 4($sp)\n");
			printf("	addiu $sp, $sp, %d\n", current_frame_size);
			printf("	jr $ra\n");
			printf("	nop\n");
		}
		return;
	}
	case ND_CALL: {
		// 引数を正しい順序で評価してスタックに積む
		for (int i = 0; i < node->argc && i < 4; i++) {
			gen(node->args[i]);
		}
		
		// スタックから引数をレジスタに移動（正しい順序）
		for (int i = 0; i < node->argc && i < 4; i++) {
			printf("	lw $a%d, %d($sp)\n", i, (node->argc - 1 - i) * 4);
		}
		printf("	addiu $sp, $sp, %d\n", node->argc * 4);
		
		// 関数呼び出し
		printf("	jal %s\n", node->name);
		printf("	nop\n");
		
		// 戻り値をスタックにプッシュ
		printf("	addiu $sp, $sp, -4\n");
		printf("	sw $v0, 0($sp)\n");
		return;
	}
	case ND_BLOCK:
		for (int i = 0; node->body[i]; i++) {
			gen(node->body[i]);
			// ブロック内の各文の結果をスタックから除去
			if (node->body[i]->kind != ND_RETURN && node->body[i]->kind != ND_IF && 
			    node->body[i]->kind != ND_WHILE && node->body[i]->kind != ND_FOR && 
			    node->body[i]->kind != ND_BLOCK) {
				printf("	lw $t0, 0($sp)\n");
				printf("	addiu $sp, $sp, 4\n");
			}
		}
		return;
	case ND_IF: {
		int seq = label_count++;
		gen(node->cond);
		printf("	lw $t0, 0($sp)\n");
		printf("	addiu $sp, $sp, 4\n");
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
		printf("	addiu $sp, $sp, 4\n");
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
			printf("	addiu $sp, $sp, 4\n");
		}
		printf(".L_begin_%d:\n", seq);
		// 条件チェック
		if (node->cond) {
			gen(node->cond);
			printf("	lw $t0, 0($sp)\n");
			printf("	addiu $sp, $sp, 4\n");
			printf("	beq $t0, $zero, .L_end_%d\n", seq);
		}
		// ボディ実行
		gen(node->then);
		// インクリメント
		if (node->inc) {
			gen(node->inc);
			printf("	lw $t0, 0($sp)\n");
			printf("	addiu $sp, $sp, 4\n");
		}
		printf("	j .L_begin_%d\n", seq);
		printf(".L_end_%d:\n", seq);
		return;
	}
	case ND_RETURN: {
		gen(node->lhs);
		printf("	lw $v0, 0($sp)\n");
		printf("	addiu $sp, $sp, 4\n");
		
		// 統一された関数エピローグ
		printf("	move $sp, $s8\n");
		printf("	lw $ra, 0($sp)\n");
		printf("	lw $s8, 4($sp)\n");
		printf("	addiu $sp, $sp, %d\n", current_frame_size);
		if (current_func_name && strcmp(current_func_name, "main") == 0) {
			printf("	move $a0, $v0\n");
			printf("	li $v0, 4001\n");
			printf("	syscall\n");
		} else {
			printf("	jr $ra\n");
			printf("	nop\n");
		}
		return;
	}
	case ND_NUM:
		printf("	li $t0, %d\n", node->val);
		printf("	addiu $sp, $sp, -4\n");
		printf("	sw $t0, 0($sp)\n");
		return;
	case ND_LVAR:
		// フレームポインタからの相対オフセットで読み込み
		printf("	lw $t0, %d($s8)\n", node->offset);
		printf("	addiu $sp, $sp, -4\n");
		printf("	sw $t0, 0($sp)\n");
		return;
	case ND_ASSIGN:
		gen_lval(node->lhs);
		gen(node->rhs);
		printf("	lw $t1, 0($sp)\n");
		printf("	addiu $sp, $sp, 4\n");
		printf("	lw $t0, 0($sp)\n");
		printf("	addiu $sp, $sp, 4\n");
		printf("	sw $t1, 0($t0)\n");
		printf("	addiu $sp, $sp, -4\n");
		printf("	sw $t1, 0($sp)\n");
		return;
	}
	
	// 二項演算子の処理
	gen(node->lhs);
	gen(node->rhs);
	printf("	lw $t1, 0($sp)\n");
	printf("	addiu $sp, $sp, 4\n");
	printf("	lw $t0, 0($sp)\n");
	printf("	addiu $sp, $sp, 4\n");
	
	switch (node->kind) {
	case ND_ADD:
		printf("	add $t0, $t0, $t1\n");
		break;
	case ND_SUB:
		printf("	sub $t0, $t0, $t1\n");
		break;
	case ND_MUL:
		printf("	mul $t0, $t0, $t1\n");
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

	printf("	addiu $sp, $sp, -4\n");
	printf("	sw $t0, 0($sp)\n");
}