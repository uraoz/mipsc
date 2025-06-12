#include "mipsc.h"

// 複合代入演算子の共通処理
void gen_compound_assign(Node* node, const char* operation, bool is_div) {
	gen_lval(node->lhs);  // 左辺のアドレスをスタックに積む
	gen_lval(node->lhs);  // 左辺のアドレスをもう一度積む（値取得用）
	printf("	lw $t0, 0($sp)\n");  // アドレスを取得
	printf("	lw $t0, 0($t0)\n");  // 現在の値を取得
	printf("	sw $t0, 0($sp)\n");  // 現在の値をスタックに格納
	gen(node->rhs);       // 右辺を評価
	printf("	lw $t1, 0($sp)\n");  // 右辺の値
	printf("	addiu $sp, $sp, 4\n");
	printf("	lw $t0, 0($sp)\n");  // 左辺の現在値
	printf("	addiu $sp, $sp, 4\n");
	
	if (is_div) {
		printf("	div $t0, $t1\n");       // 除算
		printf("	mflo $t1\n");           // 商を取得
	} else {
		printf("	%s $t1, $t0, $t1\n", operation);  // 演算
	}
	
	printf("	lw $t0, 0($sp)\n");  // 左辺のアドレス
	printf("	addiu $sp, $sp, 4\n");
	printf("	sw $t1, 0($t0)\n");  // 結果を格納
	printf("	addiu $sp, $sp, -4\n");
	printf("	sw $t1, 0($sp)\n");  // 結果をスタックに残す
}

// インクリメント/デクリメント演算子の共通処理
void gen_inc_dec(Node* node, int delta, bool is_prefix) {
	gen_lval(node->lhs);  // 変数のアドレスをスタックに積む
	printf("	lw $t0, 0($sp)\n");  // アドレスを取得
	printf("	lw $t1, 0($t0)\n");  // 現在の値を取得
	
	if (is_prefix) {
		// 前置: 先に値を更新
		printf("	addiu $t1, $t1, %d\n", delta);  // 値を更新
		printf("	sw $t1, 0($t0)\n");  // 新しい値を格納
		printf("	sw $t1, 0($sp)\n");  // 新しい値をスタックに残す
	} else {
		// 後置: 元の値を残して値を更新
		printf("	addiu $t2, $t1, %d\n", delta);  // 新しい値を$t2に計算
		printf("	sw $t2, 0($t0)\n");  // 新しい値を格納
		printf("	sw $t1, 0($sp)\n");  // 元の値をスタックに残す
	}
}

// 変数アクセスの共通処理（配列判定含む）
void gen_variable_access(Node* node, bool is_local) {
	bool is_array = false;
	
	// 型情報を確認して配列かどうか判定
	if (is_local) {
		for (LVar* var = locals; var; var = var->next) {
			if (var->offset == node->offset) {
				is_array = (var->type->ty == TY_ARRAY);
				break;
			}
		}
	} else {
		for (GVar* var = globals; var; var = var->next) {
			if (strcmp(var->name, node->name) == 0) {
				is_array = (var->type->ty == TY_ARRAY);
				break;
			}
		}
	}
	
	if (is_array) {
		// 配列の場合はアドレス（配列→ポインタ変換）
		if (is_local) {
			printf("	addiu $t0, $s8, %d\n", node->offset);
		} else {
			printf("	la $t0, %s\n", node->name);
		}
	} else {
		// 通常の変数の場合は値を読み込み
		if (is_local) {
			printf("	lw $t0, %d($s8)\n", node->offset);
		} else {
			printf("	lw $t0, %s\n", node->name);
		}
	}
	
	printf("	addiu $sp, $sp, -4\n");
	printf("	sw $t0, 0($sp)\n");
}

void gen_lval(Node* node) {
	switch (node->kind) {
	case ND_LVAR:
		// 変数のアドレスを計算
		printf("	addiu $t0, $s8, %d\n", node->offset);
		printf("	addiu $sp, $sp, -4\n");
		printf("	sw $t0, 0($sp)\n");
		return;
	case ND_GVAR:
		// グローバル変数のアドレスを計算
		printf("	la $t0, %s\n", node->name);
		printf("	addiu $sp, $sp, -4\n");
		printf("	sw $t0, 0($sp)\n");
		return;
	case ND_DEREF:
		// *ptr の左辺値は ptr の値（アドレス）
		gen(node->lhs);
		return;
	case ND_MEMBER: {
		// obj.member の左辺値はメンバのアドレス
		gen_lval(node->lhs);  // 構造体オブジェクトのアドレスを取得
		
		// 構造体の型を取得してメンバオフセットを計算
		Type* struct_type = get_type(node->lhs);
		if (struct_type->ty != TY_STRUCT) {
			error("member access on non-struct type");
		}
		
		// メンバを検索
		Token member_tok;
		member_tok.str = node->name;
		member_tok.len = strlen(node->name);
		Member* member = find_member(struct_type->struct_def, &member_tok);
		if (!member) {
			error("undefined member");
		}
		
		// ベースアドレス + メンバオフセット
		printf("	lw $t0, 0($sp)\n");           // ベースアドレスを$t0に
		printf("	addiu $t0, $t0, %d\n", member->offset);  // メンバオフセットを追加
		printf("	sw $t0, 0($sp)\n");           // 結果をスタックに格納
		return;
	}
	default:
		error("not left value");
	}
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
		
		// 現在の関数のローカル変数を設定
		LVar* saved_locals = locals;
		if (func) {
			locals = func->locals;
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
		
		// MIPS ABI準拠のスタックフレームサイズ計算
		// レイアウト: [$ra, $s8, 引数領域, ローカル変数領域]（すべて$s8からの正のオフセット）
		int arg_save_size = node->argc > 0 ? node->argc * 4 : 0;
		int frame_size = 8 + arg_save_size + local_size;  // $ra(4) + $s8(4) + 引数 + ローカル変数
		if (frame_size % 8 != 0) {
			frame_size = ((frame_size + 7) / 8) * 8;
		}
		current_frame_size = frame_size;
		
		// 統一された関数プロローグ
		printf("	addiu $sp, $sp, -%d\n", frame_size);
		printf("	sw $ra, %d($sp)\n", frame_size - 4);  // フレーム最上部に$ra
		printf("	sw $s8, %d($sp)\n", frame_size - 8);  // その下に$s8
		printf("	addiu $s8, $sp, %d\n", frame_size - 8);  // $s8をフレームポインタ基準に
		
		// 引数をローカル領域に保存（$s8からの正のオフセット）
		if (node->argc > 0) {
			for (int i = 0; i < node->argc && i < 4; i++) {
				printf("	sw $a%d, %d($s8)\n", i, -8 - (i + 1) * 4);  // 引数を$s8の下に配置
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
			printf("	lw $ra, %d($s8)\n", 4);  // $s8+4から$ra復元
			printf("	lw $s8, 0($s8)\n");      // $s8+0から旧$s8復元
			printf("	addiu $sp, $sp, %d\n", current_frame_size);
			printf("	jr $ra\n");
			printf("	nop\n");
		}
		
		// ローカル変数を復元
		locals = saved_locals;
		return;
	}
	case ND_BUILTIN_CALL: {
		// 組み込み関数の処理
		gen_builtin_call(node);
		return;
	}
	case ND_CALL: {
		// printf関数の特別処理（リファクタリング版）
		if (strcmp(node->name, "printf") == 0) {
			gen_printf_call(node);
			return;
		}
		
		// 通常の関数呼び出し
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
		if (node->els) {
			// else節がある場合
			printf("	beq $t0, $zero, .L_else_%d\n", seq);
			gen(node->then);
			printf("	j .L_end_%d\n", seq);
			printf(".L_else_%d:\n", seq);
			gen(node->els);
			printf(".L_end_%d:\n", seq);
		} else {
			// else節がない場合（従来通り）
			printf("	beq $t0, $zero, .L_end_%d\n", seq);
			gen(node->then);
			printf(".L_end_%d:\n", seq);
		}
		return;
	}
	case ND_WHILE: {
		int seq = label_count++;
		int break_label = seq;
		int continue_label = seq;
		
		// ループラベルをスタックにプッシュ
		push_loop_labels(break_label, continue_label);
		
		printf(".L_begin_%d:\n", seq);
		printf(".Lcontinue%d:\n", continue_label);  // continue先
		gen(node->cond);
		printf("	lw $t0, 0($sp)\n");
		printf("	addiu $sp, $sp, 4\n");
		printf("	beq $t0, $zero, .Lbreak%d\n", break_label);  // break先
		gen(node->then);
		printf("	j .L_begin_%d\n", seq);
		printf(".Lbreak%d:\n", break_label);  // break先ラベル
		
		// ループラベルをスタックからポップ
		pop_loop_labels();
		return;
	}
	case ND_FOR: {
		int seq = label_count++;
		int break_label = seq;
		int continue_label = label_count++;  // for文のcontinueは別ラベル
		
		// ループラベルをスタックにプッシュ
		push_loop_labels(break_label, continue_label);
		
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
			printf("	beq $t0, $zero, .Lbreak%d\n", break_label);
		}
		// ボディ実行
		gen(node->then);
		// continue先ラベル（インクリメント処理）
		printf(".Lcontinue%d:\n", continue_label);
		if (node->inc) {
			gen(node->inc);
			printf("	lw $t0, 0($sp)\n");
			printf("	addiu $sp, $sp, 4\n");
		}
		printf("	j .L_begin_%d\n", seq);
		printf(".Lbreak%d:\n", break_label);
		
		// ループラベルをスタックからポップ
		pop_loop_labels();
		return;
	}
	case ND_RETURN: {
		if (node->lhs) {
			// 戻り値がある場合
			gen(node->lhs);
			printf("	lw $v0, 0($sp)\n");
			printf("	addiu $sp, $sp, 4\n");
		} else {
			// void関数のreturn;の場合
			printf("	li $v0, 0\n");
		}
		
		// 統一された関数エピローグ
		printf("	lw $ra, %d($s8)\n", 4);  // $s8+4から$ra復元
		printf("	lw $s8, 0($s8)\n");      // $s8+0から旧$s8復元
		printf("	addiu $sp, $sp, %d\n", current_frame_size);
		printf("	jr $ra\n");
		printf("	nop\n");
		return;
	}
	case ND_NUM:
		printf("	li $t0, %d\n", node->val);
		printf("	addiu $sp, $sp, -4\n");
		printf("	sw $t0, 0($sp)\n");
		return;
	case ND_STR: {
		// 文字列リテラルのラベルを生成
		int id = string_count++;
		printf("	la $t0, .L_str_%d\n", id);
		printf("	addiu $sp, $sp, -4\n");
		printf("	sw $t0, 0($sp)\n");
		
		// 文字列データをリストに登録
		StringLiteral* str_lit = calloc(1, sizeof(StringLiteral));
		str_lit->data = calloc(node->str_len + 1, sizeof(char));
		memcpy(str_lit->data, node->str, node->str_len);
		str_lit->data[node->str_len] = '\0';
		str_lit->len = node->str_len;
		str_lit->id = id;
		str_lit->next = string_literals;
		string_literals = str_lit;
		
		return;
	}
	case ND_LVAR: {
		gen_variable_access(node, true);
		return;
	}
	case ND_GVAR: {
		gen_variable_access(node, false);
		return;
	}
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
	case ND_ADD_ASSIGN:
		gen_compound_assign(node, "add", false);
		return;
	case ND_SUB_ASSIGN:
		gen_compound_assign(node, "sub", false);
		return;
	case ND_MUL_ASSIGN:
		gen_compound_assign(node, "mul", false);
		return;
	case ND_DIV_ASSIGN:
		gen_compound_assign(node, "", true);  // 除算は特別処理
		return;
	case ND_ADDR:
		// &variable: 変数のアドレスをスタックにプッシュ
		gen_lval(node->lhs);
		return;
	case ND_DEREF:
		// *ptr: ポインタが指す値をロード
		gen(node->lhs);  // ポインタの値（アドレス）を取得
		printf("	lw $t0, 0($sp)\n");        // アドレスを$t0に取得
		printf("	lw $t0, 0($t0)\n");        // そのアドレスの内容を$t0に取得
		printf("	sw $t0, 0($sp)\n");        // 結果をスタックに格納
		return;
	case ND_PRE_INC:
		gen_inc_dec(node, 1, true);
		return;
	case ND_POST_INC:
		gen_inc_dec(node, 1, false);
		return;
	case ND_PRE_DEC:
		gen_inc_dec(node, -1, true);
		return;
	case ND_POST_DEC:
		gen_inc_dec(node, -1, false);
		return;
	case ND_NOT: {
		// 論理NOT (単項演算子)
		int label = label_count++;
		gen(node->lhs);
		printf("	lw $t0, 0($sp)\n");
		printf("	addiu $sp, $sp, 4\n");
		printf("	beqz $t0, .Ltrue%d\n", label);
		printf("	li $t0, 0\n");  // 値が0でない場合は0を返す
		printf("	j .Lend%d\n", label);
		printf(".Ltrue%d:\n", label);
		printf("	li $t0, 1\n");  // 値が0の場合は1を返す
		printf(".Lend%d:\n", label);
		printf("	addiu $sp, $sp, -4\n");
		printf("	sw $t0, 0($sp)\n");
		return;
	}
	case ND_TERNARY: {
		// 三項演算子 cond ? then_expr : else_expr
		int label = label_count++;
		
		// 条件式を評価
		gen(node->cond);
		printf("	lw $t0, 0($sp)\n");
		printf("	addiu $sp, $sp, 4\n");
		printf("	beqz $t0, .Lelse%d\n", label);  // 条件が偽なら else_expr へ
		
		// then_expr を評価
		gen(node->then);
		printf("	j .Lend%d\n", label);  // 評価後は終了へ
		
		// else_expr を評価
		printf(".Lelse%d:\n", label);
		gen(node->els);
		
		printf(".Lend%d:\n", label);
		return;
	}
	case ND_BREAK: {
		// 現在のループのbreak先にジャンプ
		if (!loop_stack) {
			error("break statement not within a loop");
		}
		printf("	j .Lbreak%d\n", loop_stack->break_label);
		return;
	}
	case ND_CONTINUE: {
		// 現在のループのcontinue先にジャンプ
		if (!loop_stack) {
			error("continue statement not within a loop");
		}
		printf("	j .Lcontinue%d\n", loop_stack->continue_label);
		return;
	}
	case ND_MEMBER: {
		// メンバアクセス: obj.member
		gen_lval(node);  // メンバのアドレスを取得
		printf("	lw $t0, 0($sp)\n");     // アドレスを$t0に
		printf("	lw $t0, 0($t0)\n");     // メンバの値を$t0に読み込み
		printf("	sw $t0, 0($sp)\n");     // 結果をスタックに格納
		return;
	}
	}
	
	// 二項演算子の処理
	gen(node->lhs);
	gen(node->rhs);
	printf("	lw $t1, 0($sp)\n");
	printf("	addiu $sp, $sp, 4\n");
	printf("	lw $t0, 0($sp)\n");
	printf("	addiu $sp, $sp, 4\n");
	
	switch (node->kind) {
	case ND_ADD: {
		// ポインタ演算のチェック
		Type* left_type = get_type(node->lhs);
		Type* right_type = get_type(node->rhs);
		
		if (left_type->ty == TY_PTR || left_type->ty == TY_ARRAY) {
			// 左がポインタ/配列: ptr + int => ptr + (int * sizeof(pointee))
			int elem_size = size_of(left_type->ptr_to);
			if (elem_size > 1) {
				printf("	li $t2, %d\n", elem_size);
				printf("	mul $t1, $t1, $t2\n");
			}
		} else if (right_type->ty == TY_PTR || right_type->ty == TY_ARRAY) {
			// 右がポインタ/配列: int + ptr => (int * sizeof(pointee)) + ptr
			int elem_size = size_of(right_type->ptr_to);
			if (elem_size > 1) {
				printf("	li $t2, %d\n", elem_size);
				printf("	mul $t0, $t0, $t2\n");
			}
		}
		
		printf("	add $t0, $t0, $t1\n");
		break;
	}
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
	case ND_MOD:
		printf("	div $t0, $t1\n");
		printf("	mfhi $t0\n");
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
	case ND_AND: {
		// 論理AND (短絡評価)
		// 左辺が偽なら右辺を評価せずに0を返す
		int label = label_count++;
		gen(node->lhs);
		printf("	lw $t0, 0($sp)\n");
		printf("	addiu $sp, $sp, 4\n");
		printf("	beqz $t0, .Lfalse%d\n", label);
		gen(node->rhs);
		printf("	lw $t0, 0($sp)\n");
		printf("	addiu $sp, $sp, 4\n");
		printf("	sltu $t0, $zero, $t0\n"); // 0 < $t0 なら1, それ以外なら0
		printf("	j .Lend%d\n", label);
		printf(".Lfalse%d:\n", label);
		printf("	li $t0, 0\n");
		printf(".Lend%d:\n", label);
		printf("	addiu $sp, $sp, -4\n");
		printf("	sw $t0, 0($sp)\n");
		return;
	}
	case ND_OR: {
		// 論理OR (短絡評価)
		// 左辺が真なら右辺を評価せずに1を返す
		int label = label_count++;
		gen(node->lhs);
		printf("	lw $t0, 0($sp)\n");
		printf("	addiu $sp, $sp, 4\n");
		printf("	bnez $t0, .Ltrue%d\n", label);
		gen(node->rhs);
		printf("	lw $t0, 0($sp)\n");
		printf("	addiu $sp, $sp, 4\n");
		printf("	sltu $t0, $zero, $t0\n"); // 0 < $t0 なら1, それ以外なら0
		printf("	j .Lend%d\n", label);
		printf(".Ltrue%d:\n", label);
		printf("	li $t0, 1\n");
		printf(".Lend%d:\n", label);
		printf("	addiu $sp, $sp, -4\n");
		printf("	sw $t0, 0($sp)\n");
		return;
	}
	}

	printf("	addiu $sp, $sp, -4\n");
	printf("	sw $t0, 0($sp)\n");
}

// =============================================================================
// printf実装の補助関数群
// =============================================================================

// writeシステムコール（1文字出力）
void gen_write_syscall(void) {
	printf("	li $a0, 1\n");              // stdout
	printf("	la $a1, .L_char_buffer\n"); // バッファアドレス
	printf("	li $a2, 1\n");              // 1バイト
	printf("	li $v0, 4004\n");           // writeシステムコール
	printf("	syscall\n");
}

// 1文字出力
void gen_printf_char(int printf_id) {
	printf(".printf_normal_char_%d:\n", printf_id);
	printf("	sb $t3, .L_char_buffer\n");  // バイト単位で格納
	gen_write_syscall();
}

// 整数出力（32bit対応）
void gen_printf_integer(int printf_id, int arg_index) {
	printf("	move $t6, $a1\n");           // 整数値を$t6に保存
	
	// 負数チェック
	printf("	bgez $t6, .printf_positive_%d_%d\n", printf_id, arg_index);
	
	// マイナス符号を出力
	printf("	li $t4, 45\n");             // '-' のASCII値
	printf("	sb $t4, .L_char_buffer\n");
	gen_write_syscall();
	printf("	sub $t6, $zero, $t6\n");    // 絶対値を取得
	
	printf(".printf_positive_%d_%d:\n", printf_id, arg_index);
	
	// 0の特別処理
	printf("	bnez $t6, .printf_nonzero_%d_%d\n", printf_id, arg_index);
	printf("	li $t4, 48\n");             // '0' のASCII値
	printf("	sb $t4, .L_char_buffer\n");
	gen_write_syscall();
	printf("	j .printf_int_done_%d_%d\n", printf_id, arg_index);
	
	printf(".printf_nonzero_%d_%d:\n", printf_id, arg_index);
	
	// 桁を逆順でスタックに積む
	printf("	li $t7, 0\n");              // 桁数カウンタ
	printf("	move $t5, $t6\n");          // 作業用コピー
	
	printf(".printf_digit_loop_%d_%d:\n", printf_id, arg_index);
	printf("	beqz $t5, .printf_print_digits_%d_%d\n", printf_id, arg_index);
	printf("	li $t9, 10\n");
	printf("	div $t5, $t9\n");
	printf("	mfhi $t4\n");               // 余り
	printf("	mflo $t5\n");               // 商
	printf("	addiu $t4, $t4, 48\n");     // ASCII変換
	printf("	addiu $sp, $sp, -4\n");     // スタックに積む
	printf("	sw $t4, 0($sp)\n");
	printf("	addiu $t7, $t7, 1\n");      // 桁数増加
	printf("	j .printf_digit_loop_%d_%d\n", printf_id, arg_index);
	
	// スタックから桁を取り出して出力
	printf(".printf_print_digits_%d_%d:\n", printf_id, arg_index);
	printf("	beqz $t7, .printf_int_done_%d_%d\n", printf_id, arg_index);
	printf("	lw $t4, 0($sp)\n");         // 桁を取得
	printf("	addiu $sp, $sp, 4\n");
	printf("	sb $t4, .L_char_buffer\n");
	gen_write_syscall();
	printf("	addiu $t7, $t7, -1\n");     // 桁数減少
	printf("	j .printf_print_digits_%d_%d\n", printf_id, arg_index);
	
	printf(".printf_int_done_%d_%d:\n", printf_id, arg_index);
}

// printf関数呼び出しの生成（リファクタリング版）
void gen_printf_call(Node* node) {
	if (node->argc < 1) {
		printf("	li $t0, 0\n");
		printf("	addiu $sp, $sp, -4\n");
		printf("	sw $t0, 0($sp)\n");
		return;
	}
	
	// フォーマット文字列を取得
	gen(node->args[0]);
	printf("	lw $t8, 0($sp)\n");          // 文字列アドレス
	printf("	addiu $sp, $sp, 4\n");
	
	int printf_id = label_count++;
	
	// 引数インデックスをレジスタで管理（$t9を使用）
	printf("	li $t9, 1\n");               // 引数インデックス（1から開始）
	
	printf(".printf_loop_%d:\n", printf_id);
	printf("	lb $t3, 0($t8)\n");          // 1文字読み込み
	printf("	beq $t3, $zero, .printf_end_%d\n", printf_id);
	
	// '%'文字をチェック
	printf("	li $t4, 37\n");              // '%' のASCII値
	printf("	bne $t3, $t4, .printf_normal_char_%d\n", printf_id);
	
	// '%'の次の文字をチェック
	printf("	addiu $t8, $t8, 1\n");
	printf("	lb $t3, 0($t8)\n");
	printf("	beq $t3, $zero, .printf_end_%d\n", printf_id);
	
	// 'd'かチェック（整数出力）
	printf("	li $t4, 100\n");             // 'd' のASCII値
	printf("	bne $t3, $t4, .printf_normal_char_%d\n", printf_id);
	
	// %d が見つかった場合：対応する引数があるかチェック
	printf("	li $t4, %d\n", node->argc);  // 総引数数
	printf("	bge $t9, $t4, .printf_no_more_args_%d\n", printf_id);
	
	// 引数を動的に取得（簡易版：最大4つまで対応）
	printf("	li $t4, 1\n");
	printf("	beq $t9, $t4, .printf_arg1_%d\n", printf_id);
	printf("	li $t4, 2\n");
	printf("	beq $t9, $t4, .printf_arg2_%d\n", printf_id);
	printf("	li $t4, 3\n");
	printf("	beq $t9, $t4, .printf_arg3_%d\n", printf_id);
	printf("	j .printf_no_more_args_%d\n", printf_id);
	
	// 各引数を個別に処理
	printf(".printf_arg1_%d:\n", printf_id);
	if (node->argc > 1) {
		gen(node->args[1]);
		printf("	lw $a1, 0($sp)\n");
		printf("	addiu $sp, $sp, 4\n");
		gen_printf_integer(printf_id, 1);
	}
	printf("	addiu $t9, $t9, 1\n");       // 引数インデックス増加
	printf("	j .printf_continue_%d\n", printf_id);
	
	printf(".printf_arg2_%d:\n", printf_id);
	if (node->argc > 2) {
		gen(node->args[2]);
		printf("	lw $a1, 0($sp)\n");
		printf("	addiu $sp, $sp, 4\n");
		gen_printf_integer(printf_id, 2);
	}
	printf("	addiu $t9, $t9, 1\n");
	printf("	j .printf_continue_%d\n", printf_id);
	
	printf(".printf_arg3_%d:\n", printf_id);
	if (node->argc > 3) {
		gen(node->args[3]);
		printf("	lw $a1, 0($sp)\n");
		printf("	addiu $sp, $sp, 4\n");
		gen_printf_integer(printf_id, 3);
	}
	printf("	addiu $t9, $t9, 1\n");
	printf("	j .printf_continue_%d\n", printf_id);
	
	printf(".printf_no_more_args_%d:\n", printf_id);
	// 引数がない場合は'0'を出力
	printf("	li $t4, 48\n");              // '0' のASCII値
	printf("	sb $t4, .L_char_buffer\n");
	gen_write_syscall();
	printf("	j .printf_continue_%d\n", printf_id);
	
	// 通常文字出力
	gen_printf_char(printf_id);
	
	printf(".printf_continue_%d:\n", printf_id);
	printf("	addiu $t8, $t8, 1\n");       // 次の文字へ
	printf("	j .printf_loop_%d\n", printf_id);
	
	printf(".printf_end_%d:\n", printf_id);
	
	// 戻り値をスタックにプッシュ
	printf("	li $t0, 0\n");
	printf("	addiu $sp, $sp, -4\n");
	printf("	sw $t0, 0($sp)\n");
}

// =============================================================================
// 組み込み関数実装
// =============================================================================

// 組み込み関数呼び出しの総合処理
void gen_builtin_call(Node* node) {
	switch (node->builtin_kind) {
	case BUILTIN_PUTCHAR:
		gen_putchar(node);
		break;
	case BUILTIN_GETCHAR:
		gen_getchar(node);
		break;
	case BUILTIN_PUTS:
		gen_puts(node);
		break;
	case BUILTIN_STRLEN:
		gen_strlen(node);
		break;
	case BUILTIN_STRCMP:
		gen_strcmp(node);
		break;
	case BUILTIN_STRCPY:
		gen_strcpy(node);
		break;
	default:
		error("unknown builtin function");
	}
}

// int putchar(int c) - 1文字出力
void gen_putchar(Node* node) {
	if (node->argc != 1) {
		error("putchar requires exactly 1 argument");
	}
	
	// 引数を評価
	gen(node->args[0]);
	printf("\tlw $t0, 0($sp)\n");      // 文字コードを取得
	printf("\taddiu $sp, $sp, 4\n");
	
	// 文字をバッファに格納
	printf("\tsb $t0, .L_char_buffer\n");
	
	// writeシステムコール
	gen_write_syscall();
	
	// 戻り値として文字コードをスタックにプッシュ
	printf("\taddiu $sp, $sp, -4\n");
	printf("\tsw $t0, 0($sp)\n");
}

// int puts(const char* s) - 文字列出力
void gen_puts(Node* node) {
	if (node->argc != 1) {
		error("puts requires exactly 1 argument");
	}
	
	// 文字列ポインタを評価
	gen(node->args[0]);
	printf("\tlw $t8, 0($sp)\n");          // 文字列アドレス
	printf("\taddiu $sp, $sp, 4\n");
	
	int puts_id = label_count++;
	
	// 文字列の各文字を出力
	printf(".puts_loop_%d:\n", puts_id);
	printf("\tlb $t3, 0($t8)\n");          // 1文字読み込み
	printf("\tbeq $t3, $zero, .puts_newline_%d\n", puts_id);
	
	// 文字をバッファに格納して出力
	printf("\tsb $t3, .L_char_buffer\n");
	gen_write_syscall();
	printf("\taddiu $t8, $t8, 1\n");       // 次の文字へ
	printf("\tj .puts_loop_%d\n", puts_id);
	
	// 改行文字を出力
	printf(".puts_newline_%d:\n", puts_id);
	printf("\tli $t3, 10\n");              // n のASCII値
	printf("\tsb $t3, .L_char_buffer\n");
	gen_write_syscall();
	
	// 戻り値として0をスタックにプッシュ
	printf("\tli $t0, 0\n");
	printf("\taddiu $sp, $sp, -4\n");
	printf("\tsw $t0, 0($sp)\n");
}

// int strlen(const char* s) - 文字列長計算  
void gen_strlen(Node* node) {
	if (node->argc != 1) {
		error("strlen requires exactly 1 argument");
	}
	
	// 文字列ポインタを評価
	gen(node->args[0]);
	printf("\tlw $t8, 0($sp)\n");          // 文字列アドレス
	printf("\taddiu $sp, $sp, 4\n");
	
	int strlen_id = label_count++;
	
	// 長さカウンタを初期化
	printf("\tli $t7, 0\n");               // 長さカウンタ
	printf("\tmove $t9, $t8\n");           // 作業用ポインタ
	
	// 文字列をスキャンしてヌル文字を探す
	printf(".strlen_loop_%d:\n", strlen_id);
	printf("\tlb $t3, 0($t9)\n");          // 1文字読み込み
	printf("\tbeq $t3, $zero, .strlen_done_%d\n", strlen_id);
	printf("\taddiu $t7, $t7, 1\n");       // 長さ増加
	printf("\taddiu $t9, $t9, 1\n");       // 次の文字へ
	printf("\tj .strlen_loop_%d\n", strlen_id);
	
	printf(".strlen_done_%d:\n", strlen_id);
	
	// 戻り値をスタックにプッシュ
	printf("\taddiu $sp, $sp, -4\n");
	printf("\tsw $t7, 0($sp)\n");
}

// int getchar(void) - 1文字入力
void gen_getchar(Node* node) {
	if (node->argc != 0) {
		error("getchar requires no arguments");
	}
	
	// readシステムコール（stdin=0, buffer=.L_char_buffer, count=1）
	printf("\tli $a0, 0\n");               // stdin
	printf("\tla $a1, .L_char_buffer\n");  // バッファアドレス
	printf("\tli $a2, 1\n");               // 1バイト
	printf("\tli $v0, 4003\n");            // readシステムコール
	printf("\tsyscall\n");
	
	// バッファから文字を読み込み
	printf("\tlb $t0, .L_char_buffer\n");  // バイト単位で読み込み
	
	// 戻り値をスタックにプッシュ
	printf("\taddiu $sp, $sp, -4\n");
	printf("\tsw $t0, 0($sp)\n");
}

// int strcmp(const char* s1, const char* s2) - 文字列比較
void gen_strcmp(Node* node) {
	if (node->argc != 2) {
		error("strcmp requires exactly 2 arguments");
	}
	
	// 引数を評価
	gen(node->args[0]);                     // s1
	gen(node->args[1]);                     // s2
	printf("\tlw $t9, 0($sp)\n");          // s2
	printf("\taddiu $sp, $sp, 4\n");
	printf("\tlw $t8, 0($sp)\n");          // s1
	printf("\taddiu $sp, $sp, 4\n");
	
	int strcmp_id = label_count++;
	
	// 文字列を1文字ずつ比較
	printf(".strcmp_loop_%d:\n", strcmp_id);
	printf("\tlb $t3, 0($t8)\n");          // s1の文字
	printf("\tlb $t4, 0($t9)\n");          // s2の文字
	
	// どちらかがヌル文字か
	printf("\tbeq $t3, $zero, .strcmp_check_s2_%d\n", strcmp_id);
	printf("\tbeq $t4, $zero, .strcmp_s1_longer_%d\n", strcmp_id);
	
	// 文字が異なるか
	printf("\tbne $t3, $t4, .strcmp_different_%d\n", strcmp_id);
	
	// 次の文字へ
	printf("\taddiu $t8, $t8, 1\n");
	printf("\taddiu $t9, $t9, 1\n");
	printf("\tj .strcmp_loop_%d\n", strcmp_id);
	
	// s1がヌル文字の場合
	printf(".strcmp_check_s2_%d:\n", strcmp_id);
	printf("\tbeq $t4, $zero, .strcmp_equal_%d\n", strcmp_id);
	
	// s2が長い場合
	printf("\tli $t0, -1\n");
	printf("\tj .strcmp_done_%d\n", strcmp_id);
	
	// s1が長い場合
	printf(".strcmp_s1_longer_%d:\n", strcmp_id);
	printf("\tli $t0, 1\n");
	printf("\tj .strcmp_done_%d\n", strcmp_id);
	
	// 文字が異なる場合
	printf(".strcmp_different_%d:\n", strcmp_id);
	printf("\tsub $t0, $t3, $t4\n");       // s1[i] - s2[i]
	printf("\tj .strcmp_done_%d\n", strcmp_id);
	
	// 等しい場合
	printf(".strcmp_equal_%d:\n", strcmp_id);
	printf("\tli $t0, 0\n");
	
	printf(".strcmp_done_%d:\n", strcmp_id);
	
	// 戻り値をスタックにプッシュ
	printf("\taddiu $sp, $sp, -4\n");
	printf("\tsw $t0, 0($sp)\n");
}

// char* strcpy(char* dest, const char* src) - 文字列コピー
void gen_strcpy(Node* node) {
	if (node->argc != 2) {
		error("strcpy requires exactly 2 arguments");
	}
	
	// 引数を評価
	gen(node->args[0]);                     // dest
	gen(node->args[1]);                     // src
	printf("\tlw $t9, 0($sp)\n");          // src
	printf("\taddiu $sp, $sp, 4\n");
	printf("\tlw $t8, 0($sp)\n");          // dest
	printf("\taddiu $sp, $sp, 4\n");
	
	int strcpy_id = label_count++;
	
	// destの元の値を保存（戻り値用）
	printf("\tmove $t7, $t8\n");           // destの元のアドレス
	
	// 文字列を1文字ずつコピー
	printf(".strcpy_loop_%d:\n", strcpy_id);
	printf("\tlb $t3, 0($t9)\n");          // srcから1文字読み込み
	printf("\tsb $t3, 0($t8)\n");          // destに1文字書き込み
	printf("\tbeq $t3, $zero, .strcpy_done_%d\n", strcpy_id);  // ヌル文字で終了
	
	// 次の文字へ
	printf("\taddiu $t8, $t8, 1\n");
	printf("\taddiu $t9, $t9, 1\n");
	printf("\tj .strcpy_loop_%d\n", strcpy_id);
	
	printf(".strcpy_done_%d:\n", strcpy_id);
	
	// destのアドレスを戻り値としてスタックにプッシュ
	printf("\taddiu $sp, $sp, -4\n");
	printf("\tsw $t7, 0($sp)\n");
}

