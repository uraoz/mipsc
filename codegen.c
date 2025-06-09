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
		
		// ローカル変数領域を0クリア（コメントアウト）
		/*
		if (local_size > 0) {
			// 負のオフセットでローカル変数にアクセスするため、スタックをクリア
			printf("	li $t9, %d\n", local_size);
			printf(".L_clear_loop_%s:\n", node->name);
			printf("	beq $t9, $zero, .L_clear_end_%s\n", node->name);
			printf("	sub $t8, $s8, $t9\n");
			printf("	sw $zero, 0($t8)\n");
			printf("	addiu $t9, $t9, -4\n");
			printf("	j .L_clear_loop_%s\n", node->name);
			printf(".L_clear_end_%s:\n", node->name);
		}
		*/
		
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
	case ND_CALL: {
		// printf関数の特別処理
		if (strcmp(node->name, "printf") == 0) {
			if (node->argc >= 1) {
				// 第一引数（フォーマット文字列）を$a0に設定
				gen(node->args[0]);
				printf("	lw $a0, 0($sp)\n");
				printf("	addiu $sp, $sp, 4\n");
				
				// 文字列長を動的に計算（strlenライクな処理）
				int strlen_id = label_count++;
				printf("	move $t1, $a0\n");      // 文字列アドレスを$t1にコピー
				printf("	li $t2, 0\n");          // カウンタを0に初期化
				printf(".strlen_loop_%d:\n", strlen_id);
				printf("	lb $t3, 0($t1)\n");     // 1バイト読み込み
				printf("	beq $t3, $zero, .strlen_end_%d\n", strlen_id); // null文字なら終了
				printf("	addiu $t1, $t1, 1\n");  // アドレスを次へ
				printf("	addiu $t2, $t2, 1\n");  // カウンタをインクリメント
				printf("	j .strlen_loop_%d\n", strlen_id);
				printf("	nop\n");
				printf(".strlen_end_%d:\n", strlen_id);
				
				// MIPSシステムコール4004（write）を使用
				printf("	move $a1, $a0\n");      // 文字列アドレスを$a1に
				printf("	li $a0, 1\n");          // stdout (1) を$a0に
				printf("	move $a2, $t2\n");      // 計算された文字列長を$a2に
				printf("	li $v0, 4004\n");       // writeシステムコール
				printf("	syscall\n");
			}
			
			// printfの戻り値として適当な値（印刷した文字数など）をスタックにプッシュ
			printf("	li $t0, 0\n");
			printf("	addiu $sp, $sp, -4\n");
			printf("	sw $t0, 0($sp)\n");
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