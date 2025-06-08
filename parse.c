#include "mipsc.h"

Node* code[MAX_STATEMENTS];

// 型管理関数の実装
Type* new_type(TypeKind ty) {
	Type* type = calloc(1, sizeof(Type));
	type->ty = ty;
	return type;
}

Type* pointer_to(Type* base) {
	Type* type = new_type(TY_PTR);
	type->ptr_to = base;
	return type;
}

Type* array_to(Type* base, size_t size) {
	Type* type = new_type(TY_ARRAY);
	type->ptr_to = base;
	type->array_size = size;
	return type;
}

bool is_integer(Type* ty) {
	return ty->ty == TY_INT || ty->ty == TY_CHAR;
}

int size_of(Type* ty) {
	switch (ty->ty) {
	case TY_INT:
	case TY_PTR:
		return SIZE_INT; // MIPS32では4バイト
	case TY_CHAR:
		return SIZE_CHAR;
	case TY_ARRAY:
		// 配列のサイズ = 要素の型のサイズ × 要素数
		return size_of(ty->ptr_to) * ty->array_size;
	default:
		return SIZE_INT;
	}
}
// 次のトークンが期待される記号であればトークンを読み進める
bool consume(char* op) {
	if (token->kind != TK_RESERVED || token->len != strlen(op) || memcmp(token->str, op, token->len))
		return false;
	token = token->next;
	return true;
}
// 次のトークンが期待される記号でなければエラーを出す
// そうでなければトークンを読み進める
void expect(char* op) {
	if (token->kind != TK_RESERVED || token->len != strlen(op) || memcmp(token->str, op, token->len))
		error("'%s'is expected but not", op);
	token = token->next;
}

// 次のトークンが数値ならトークンを読み進めてその数値を返す
int expect_number() {
	if (token->kind != TK_NUM)
		error("not a number");
	int val = token->val;
	token = token->next;
	return val;
}

bool at_eof() {
	return token->kind == TK_EOF;
}

// 新しいトークンを作成してcurに繋げる
Token* new_token(TokenKind kind, Token* cur, char* str, int len) {
	Token* tok = calloc(1, sizeof(Token));
	tok->kind = kind;
	tok->str = str;
	cur->next = tok;
	tok->len = len;
	return tok;
}

bool startwith(char* p, char* str) {
	return strncmp(p, str, strlen(str)) == 0;
}
// 入力文字列pをトークンに分割して返す
Token* tokenize(char* p) {
	Token head;
	head.next = NULL;
	Token* cur = &head;
	while (*p) {
		if (isspace(*p)) {
			p++;
			continue;
		}
		
		// 単一行コメント (//)
		if (*p == '/' && *(p + 1) == '/') {
			p += 2; // "//"をスキップ
			while (*p && *p != '\n') {
				p++; // 行末まで読み飛ばし
			}
			continue;
		}
		
		// 複数行コメント (/* */)
		if (*p == '/' && *(p + 1) == '*') {
			char* comment_start = p;
			p += 2; // "/*"をスキップ
			bool found_end = false;
			while (*p) {
				if (*p == '*' && *(p + 1) == '/') {
					p += 2; // "*/"をスキップ
					found_end = true;
					break;
				}
				p++;
			}
			// 終端チェック（*/が見つからない場合）
			if (!found_end) {
				error_at(comment_start, "unterminated comment");
			}
			continue;
		}
		
		// 数値
		if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p, 0);
			char* q = p;
			cur->val = strtol(p, &p, 10);
			cur->len = p - q;
			continue;
		}
		// 文字列リテラル
		if (*p == '"') {
			char* start = p;
			p++; // 開始の"をスキップ
			
			// エスケープシーケンスを処理した文字列を作成
			char* str_buf = calloc(1, 256); // 一時的な文字列バッファ
			int str_len = 0;
			
			while (*p && *p != '"') {
				if (*p == '\\') {
					// エスケープシーケンス
					p++; // バックスラッシュをスキップ
					switch (*p) {
					case 'n': str_buf[str_len++] = '\n'; break;
					case 't': str_buf[str_len++] = '\t'; break;
					case 'r': str_buf[str_len++] = '\r'; break;
					case '\\': str_buf[str_len++] = '\\'; break;
					case '"': str_buf[str_len++] = '"'; break;
					case '0': str_buf[str_len++] = '\0'; break;
					default:
						error_at(p, "unknown escape sequence in string");
					}
					p++;
				} else {
					// 通常の文字
					str_buf[str_len++] = *p;
					p++;
				}
			}
			
			if (*p != '"') {
				error_at(start, "unterminated string literal");
			}
			p++; // 終了の"をスキップ
			
			cur = new_token(TK_STR, cur, start, p - start);
			// エスケープ処理済みの文字列をトークンに格納
			cur->str_data = str_buf;
			cur->str_len = str_len;
			continue;
		}
		
		// 文字リテラル
		if (*p == '\'') {
			char* start = p;
			p++; // 開始の'をスキップ
			
			int char_value;
			if (*p == '\\') {
				// エスケープシーケンス
				p++; // バックスラッシュをスキップ
				switch (*p) {
				case 'n': char_value = '\n'; break;
				case 't': char_value = '\t'; break;
				case 'r': char_value = '\r'; break;
				case '\\': char_value = '\\'; break;
				case '\'': char_value = '\''; break;
				case '0': char_value = '\0'; break;
				default:
					error_at(p, "unknown escape sequence");
				}
				p++;
			} else if (*p) {
				// 通常の文字
				char_value = *p;
				p++;
			} else {
				error_at(start, "unterminated character literal");
			}
			
			if (*p != '\'') {
				error_at(start, "unterminated character literal");
			}
			p++; // 終了の'をスキップ
			
			cur = new_token(TK_CHAR_LITERAL, cur, start, p - start);
			cur->val = char_value; // ASCII値を保存
			continue;
		}
		// !=演算子 (論理NOTより先に処理)
		if (startwith(p, "!=")) {
			cur = new_token(TK_RESERVED, cur, p, 2);
			p += 2;
			continue;
		}
		// 論理NOT演算子 (単独の!) - デバッグ用出力追加
		if (*p == '!') {
			fprintf(stderr, "DEBUG: Found ! at position, creating TK_NOT token\n");
			cur = new_token(TK_NOT, cur, p, 1);
			p++;
			continue;
		}
		//その他の二文字の記号
		if (startwith(p, "==") || startwith(p, "<=") || startwith(p, ">=")) {
			cur = new_token(TK_RESERVED, cur, p, 2);
			p += 2;
			continue;
		}
		// 複合代入演算子
		if (startwith(p, "+=")) {
			cur = new_token(TK_ADD_ASSIGN, cur, p, 2);
			p += 2;
			continue;
		}
		if (startwith(p, "-=")) {
			cur = new_token(TK_SUB_ASSIGN, cur, p, 2);
			p += 2;
			continue;
		}
		if (startwith(p, "*=")) {
			cur = new_token(TK_MUL_ASSIGN, cur, p, 2);
			p += 2;
			continue;
		}
		if (startwith(p, "/=")) {
			cur = new_token(TK_DIV_ASSIGN, cur, p, 2);
			p += 2;
			continue;
		}
		// インクリメント・デクリメント演算子
		if (startwith(p, "++")) {
			cur = new_token(TK_INC, cur, p, 2);
			p += 2;
			continue;
		}
		if (startwith(p, "--")) {
			cur = new_token(TK_DEC, cur, p, 2);
			p += 2;
			continue;
		}
		// 論理演算子
		if (startwith(p, "&&")) {
			cur = new_token(TK_AND, cur, p, 2);
			p += 2;
			continue;
		}
		if (startwith(p, "||")) {
			cur = new_token(TK_OR, cur, p, 2);
			p += 2;
			continue;
		}
		//一文字の記号
		if (strchr("+-*/()<>;={},&[]", *p)) {
			cur = new_token(TK_RESERVED, cur, p, 1);
			p++;
			continue;
		}
		//キーワードまたは変数（英小文字）
		if ('a' <= *p && *p <= 'z') {
			char* start = p;
			while (('a' <= *p && *p <= 'z') || ('0' <= *p && *p <= '9') || *p == '_') {
				p++;
			}
			int len = p - start;
			if (len == LEN_RETURN && !memcmp(start, "return", LEN_RETURN)) {
				cur = new_token(TK_RETURN, cur, start, len);
			} else if (len == LEN_IF && !memcmp(start, "if", LEN_IF)) {
				cur = new_token(TK_IF, cur, start, len);
			} else if (len == LEN_ELSE && !memcmp(start, "else", LEN_ELSE)) {
				cur = new_token(TK_ELSE, cur, start, len);
			} else if (len == LEN_WHILE && !memcmp(start, "while", LEN_WHILE)) {
				cur = new_token(TK_WHILE, cur, start, len);
			} else if (len == LEN_FOR && !memcmp(start, "for", LEN_FOR)) {
				cur = new_token(TK_FOR, cur, start, len);
			} else if (len == LEN_INT && !memcmp(start, "int", LEN_INT)) {
				cur = new_token(TK_INT, cur, start, len);
			} else if (len == LEN_CHAR && !memcmp(start, "char", LEN_CHAR)) {
				cur = new_token(TK_CHAR, cur, start, len);
			} else if (len == LEN_SIZEOF && !memcmp(start, "sizeof", LEN_SIZEOF)) {
				cur = new_token(TK_SIZEOF, cur, start, len);
			} else {
				cur = new_token(TK_IDENT, cur, start, len);
			}
			cur->len = len;
			continue;
		}
		
		error_at(p, "invalid character");
	}
	new_token(TK_EOF, cur, p, 0);
	return head.next;
}

// 変数を名前で検索する。見つからなかった場合はNULLを返す。
LVar* find_lvar(Token* tok) {
	for (LVar* var = locals; var; var = var->next) {
		if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
			return var;
		}
	}
	return NULL;
}

// グローバル変数を名前で検索する。見つからなかった場合はNULLを返す。
GVar* find_gvar(Token* tok) {
	for (GVar* var = globals; var; var = var->next) {
		if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
			return var;
		}
	}
	return NULL;
}

Token* consume_ident() {
	if (token->kind != TK_IDENT)
		return NULL;
	Token* tok = token;
	token = token->next;
	return tok;
}

bool consume_return() {
	if (token->kind != TK_RETURN)
		return false;
	token = token->next;
	return true;
}

bool consume_if() {
	if (token->kind != TK_IF)
		return false;
	token = token->next;
	return true;
}

bool consume_while() {
	if (token->kind != TK_WHILE)
		return false;
	token = token->next;
	return true;
}

bool consume_for() {
	if (token->kind != TK_FOR)
		return false;
	token = token->next;
	return true;
}

bool consume_int() {
	if (token->kind != TK_INT)
		return false;
	token = token->next;
	return true;
}

bool consume_char() {
	if (token->kind != TK_CHAR)
		return false;
	token = token->next;
	return true;
}

bool consume_sizeof() {
	if (token->kind != TK_SIZEOF)
		return false;
	token = token->next;
	return true;
}

// 基本型（int, char）をパースする関数。ポインタの*はパースしない。
Type* parse_type_prefix() {
	Type* base_type;
	
	if (consume_int()) {
		base_type = new_type(TY_INT);
	} else if (consume_char()) {
		base_type = new_type(TY_CHAR);
	} else {
		error("type expected");
	}
	
	return base_type;
}

// 型をパースする関数（int, char, int*, int**, etc.）
Type* parse_type() {
	Type* base_type = parse_type_prefix();
	
	// ポインタレベルをカウント（*, **, *** など）
	while (consume("*")) {
		base_type = pointer_to(base_type);
	}
	
	return base_type;
}

// ノードから型を推定する関数
Type* get_type(Node* node) {
	switch (node->kind) {
	case ND_NUM:
		return new_type(TY_INT);
	case ND_LVAR: {
		// 変数の型を検索
		for (LVar* var = locals; var; var = var->next) {
			if (var->offset == node->offset) {
				return var->type;
			}
		}
		return new_type(TY_INT);  // デフォルト
	}
	case ND_GVAR: {
		// グローバル変数の型を検索
		for (GVar* var = globals; var; var = var->next) {
			if (strcmp(var->name, node->name) == 0) {
				return var->type;
			}
		}
		return new_type(TY_INT);  // デフォルト
	}
	case ND_ADDR: {
		// &expr の型は expr* 
		Type* base = get_type(node->lhs);
		return pointer_to(base);
	}
	case ND_DEREF: {
		// *ptr の型は ptr が指す型
		Type* ptr_type = get_type(node->lhs);
		if (ptr_type->ty == TY_PTR) {
			return ptr_type->ptr_to;
		}
		return new_type(TY_INT);  // エラーの場合
	}
	case ND_ADD:
	case ND_SUB:
	case ND_MUL:
	case ND_DIV:
		return new_type(TY_INT);  // 算術演算の結果はint
	default:
		return new_type(TY_INT);  // デフォルト
	}
}


// 新しいノードを作成する関数
Node* new_node(NodeKind kind, Node* lhs, Node* rhs) {
	Node* node = calloc(1, sizeof(Node));
	node->kind = kind;
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

Node* new_node_num(int val) {
	Node* node = calloc(1, sizeof(Node));
	node->kind = ND_NUM;
	node->val = val;
	return node;
}


// ノードのパーサ
/*
program    = stmt*
stmt       = "return" expr ";" | "if" "(" expr ")" stmt | "while" "(" expr ")" stmt | "for" "(" expr? ";" expr? ";" expr? ")" stmt | "{" stmt* "}" | expr ";"
expr       = assign
assign     = equality ("=" assign)?
equality   = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add        = mul ("+" mul | "-" mul)*
mul        = unary ("*" unary | "/" unary)*
unary      = ("+" | "-")? primary
primary    = num | ident | "(" expr ")"
*/

// セミコロン区切りの文をパースする関数

Node* assign() {
	Node* node = logical_or();
	if (consume("=")) {
		node = new_node(ND_ASSIGN, node, assign());
	} else if (token->kind == TK_ADD_ASSIGN) {
		token = token->next;
		node = new_node(ND_ADD_ASSIGN, node, assign());
	} else if (token->kind == TK_SUB_ASSIGN) {
		token = token->next;
		node = new_node(ND_SUB_ASSIGN, node, assign());
	} else if (token->kind == TK_MUL_ASSIGN) {
		token = token->next;
		node = new_node(ND_MUL_ASSIGN, node, assign());
	} else if (token->kind == TK_DIV_ASSIGN) {
		token = token->next;
		node = new_node(ND_DIV_ASSIGN, node, assign());
	}
	return node;
}
Node* expr() {
	return assign();
}
Node* stmt() {
	Node* node;
	if (token->kind == TK_INT || token->kind == TK_CHAR) {
		// 変数宣言: int varname; int* ptr; char* str; int arr[10]; など
		Type* type = parse_type();
		Token* tok = consume_ident();
		if (!tok) {
			error("variable name expected");
		}
		
		// 配列宣言のチェック: int name[size]
		if (consume("[")) {
			int array_size = expect_number();
			expect("]");
			type = array_to(type, array_size);
		}
		
		// 新しい変数をローカル変数リストに追加
		LVar* lvar = calloc(1, sizeof(LVar));
		lvar->next = locals;
		lvar->name = tok->str;
		lvar->len = tok->len;
		lvar->type = type;
		
		// 現在の関数のローカル変数のオフセットを計算（型のサイズを考慮）
		int min_offset = ARG_SAVE_OFFSET;  // 引数保存領域の下から開始
		for (LVar* v = locals; v; v = v->next) {
			if (v->offset < min_offset) {
				min_offset = v->offset;
			}
		}
		// 型のサイズでアライメント調整（とりあえず4バイト境界）
		int var_size = size_of(type);
		if (var_size < MIN_ALIGNMENT) var_size = MIN_ALIGNMENT; // 最小4バイトでアライメント
		lvar->offset = min_offset - var_size;
		locals = lvar;
		
		// 初期化があるかチェック
		if (consume("=")) {
			// int x = 10; の形式
			Node* var_node = calloc(1, sizeof(Node));
			var_node->kind = ND_LVAR;
			var_node->offset = lvar->offset;
			var_node->type = type;
			
			Node* init_expr = expr();
			node = new_node(ND_ASSIGN, var_node, init_expr);
			expect(";");
		} else {
			// int x; の形式（初期化なし）
			expect(";");
			// 空のノードを返す（宣言は実行時に何もしない）
			node = calloc(1, sizeof(Node));
			node->kind = ND_NUM;
			node->val = 0;
		}
		return node;
	}
	if (consume_return()) {
		node = calloc(1, sizeof(Node));
		node->kind = ND_RETURN;
		node->lhs = expr();
		expect(";");
		return node;
	}
	if (consume_if()) {
		node = calloc(1, sizeof(Node));
		node->kind = ND_IF;
		expect("(");
		node->cond = expr();
		expect(")");
		node->then = stmt();
		if (token->kind == TK_ELSE) {
			token = token->next; // "else"をスキップ
			node->els = stmt();
		}
		return node;
	}
	if (consume_while()) {
		node = calloc(1, sizeof(Node));
		node->kind = ND_WHILE;
		expect("(");
		node->cond = expr();
		expect(")");
		node->then = stmt();
		return node;
	}
	if (consume_for()) {
		node = calloc(1, sizeof(Node));
		node->kind = ND_FOR;
		expect("(");
		// 初期化部分（省略可能）
		if (!consume(";")) {
			node->init = expr();
			expect(";");
		}
		// 条件部分（省略可能）
		if (!consume(";")) {
			node->cond = expr();
			expect(";");
		}
		// インクリメント部分（省略可能）
		if (!consume(")")) {
			node->inc = expr();
			expect(")");
		}
		node->then = stmt();
		return node;
	}
	if (consume("{")) {
		node = calloc(1, sizeof(Node));
		node->kind = ND_BLOCK;
		// 文のリストを格納する配列を動的確保
		Node** stmts = malloc(sizeof(Node*) * MAX_STATEMENTS); // 最大100文
		int i = 0;
		while (!consume("}")) {
			stmts[i++] = stmt();
		}
		stmts[i] = NULL; // 終端
		node->body = stmts;
		return node;
	}
	// 空文の処理
	if (consume(";")) {
		node = calloc(1, sizeof(Node));
		node->kind = ND_NUM;
		node->val = 0;
		return node;
	}
	node = expr();
	expect(";");
	return node;
}
// 関数定義をパースする
Node* function() {
	// 型 functionname(params) { ... } の形式をパース
	Type* return_type = parse_type();
	
	Token* tok = consume_ident();
	if (!tok) {
		error("function name expected");
	}
	
	// 関数名をコピー
	char* fname = calloc(tok->len + 1, sizeof(char));
	memcpy(fname, tok->str, tok->len);
	
	expect("(");
	
	// 引数の解析（パラメータ）
	LVar* params = NULL;
	int argc = 0;
	if (!consume(")")) {
		do {
			Type* param_type = parse_type();
			Token* param_tok = consume_ident();
			if (!param_tok) {
				error("parameter name expected");
			}
			
			// パラメータをローカル変数リストに追加
			LVar* param = calloc(1, sizeof(LVar));
			param->next = params;
			param->name = param_tok->str;
			param->len = param_tok->len;
			param->type = param_type;
			param->offset = ARG_SAVE_OFFSET - (argc + 1) * ARG_SIZE; // 引数は$s8の下の領域に
			params = param;
			argc++;
		} while (consume(","));
		expect(")");
	}
	
	expect("{");
	
	// この関数のローカル変数を設定
	LVar* saved_locals = locals;
	locals = params;
	
	// 関数ノードを作成
	Node* node = calloc(1, sizeof(Node));
	node->kind = ND_FUNC;
	node->name = fname;
	node->argc = argc;
	
	// 文のリストを格納する配列を動的確保
	Node** stmts = malloc(sizeof(Node*) * MAX_STATEMENTS); // 最大100文
	int i = 0;
	while (!consume("}")) {
		stmts[i++] = stmt();
	}
	stmts[i] = NULL; // 終端
	node->body = stmts;
	
	// 関数をリストに追加
	Function* func = calloc(1, sizeof(Function));
	func->next = functions;
	func->name = fname;
	func->len = strlen(fname);
	func->node = node;
	func->locals = locals;
	functions = func;
	
	// ローカル変数を復元
	locals = saved_locals;
	
	return node;
}

void program() {
	int i = 0;
	while (!at_eof()) {
		// 型プレフィックスを先読み
		Token* saved_token = token;
		Type* type_prefix = parse_type_prefix();
		
		// ポインタレベルをカウント
		while (consume("*")) {
			type_prefix = pointer_to(type_prefix);
		}
		
		// 識別子を読む
		Token* name_tok = consume_ident();
		if (!name_tok) {
			error("identifier expected");
		}
		
		// 次のトークンで関数か変数かを判定
		if (consume("(")) {
			// 関数定義 - トークンを元に戻して関数パースを呼ぶ
			token = saved_token;
			code[i++] = function();
		} else {
			// グローバル変数宣言
			Type* var_type = type_prefix;
			
			// 配列宣言のチェック: int name[size]
			if (consume("[")) {
				int array_size = expect_number();
				expect("]");
				var_type = array_to(var_type, array_size);
			}
			
			// グローバル変数をリストに追加
			GVar* gvar = calloc(1, sizeof(GVar));
			gvar->next = globals;
			gvar->name = calloc(name_tok->len + 1, sizeof(char));
			memcpy(gvar->name, name_tok->str, name_tok->len);
			gvar->len = name_tok->len;
			gvar->type = var_type;
			globals = gvar;
			
			expect(";");
		}
	}
	code[i] = NULL;
}
// 等しいと不等しいのノードを作成する関数
// 論理OR演算子
Node* logical_or() {
	Node* node = logical_and();

	while (1) {
		if (token->kind == TK_OR) {
			token = token->next;
			node = new_node(ND_OR, node, logical_and());
		} else {
			return node;
		}
	}
}

// 論理AND演算子
Node* logical_and() {
	Node* node = equality();

	while (1) {
		if (token->kind == TK_AND) {
			token = token->next;
			node = new_node(ND_AND, node, equality());
		} else {
			return node;
		}
	}
}

Node* equality() {
	Node* node = relational();

	while (1) {
		if (consume("=="))
			node = new_node(ND_EQ, node, relational());
		else if (consume("!="))
			node = new_node(ND_NE, node, relational());
		else
			return node;
	}
}
// 大なりと小なりのノードを作成する関数
Node* relational() {
	Node* node = add();
	while (1) {
		if (consume("<"))
			node = new_node(ND_LT, node, add());
		else if (consume("<="))
			node = new_node(ND_LE, node, add());
		else if (consume(">"))
			node = new_node(ND_LT, add(), node);
		else if (consume(">="))
			node = new_node(ND_LE, add(), node);
		else
			return node;
	}
}
// 加算と減算のノードを作成する関数
Node* add() {
	Node* node = mul();
	while (1) {
		if (consume("+")) {
			node = new_node(ND_ADD, node, mul());
			continue;
		}
		if (consume("-")) {
			node = new_node(ND_SUB, node, mul());
			continue;
		}
		break;
	}
	return node;
}
// 乗算と除算のノードを作成する関数
Node* mul() {
	Node* node = unary();
	while (1) {
		if (consume("*")) {
			node = new_node(ND_MUL, node, unary());
			continue;
		}
		if (consume("/")) {
			node = new_node(ND_DIV, node, unary());
			continue;
		}
		break;
	}
	return node;
}
// 数値または'('で始まるノードを作成する関数
Node* primary() {
	if (consume_sizeof()) {
		expect("(");
		
		// sizeof(型) の場合
		if (token->kind == TK_INT || token->kind == TK_CHAR) {
			Type* ty = parse_type();
			expect(")");
			
			// 型のサイズを定数ノードとして返す
			return new_node_num(size_of(ty));
		}
		
		// sizeof(式) の場合
		Node* node = expr();
		expect(")");
		
		// 式の型からサイズを計算
		Type* ty = get_type(node);
		return new_node_num(size_of(ty));
	}
	if (consume("(")) {
		Node* node = expr();
		expect(")");
		return node;
	}
	if (token->kind == TK_NUM) {
		return new_node_num(expect_number());
	}
	if (token->kind == TK_CHAR_LITERAL) {
		int val = token->val;
		token = token->next;
		return new_node_num(val);
	}
	if (token->kind == TK_STR) {
		Token* tok = token;
		token = token->next;
		
		Node* node = calloc(1, sizeof(Node));
		node->kind = ND_STR;
		
		// エスケープ処理済みの文字列データをコピー
		node->str_len = tok->str_len;
		node->str = calloc(node->str_len + 1, sizeof(char));
		memcpy(node->str, tok->str_data, node->str_len);
		node->str[node->str_len] = '\0';
		
		// 文字列リテラルの型はchar*
		Type* ty = calloc(1, sizeof(Type));
		ty->ty = TY_PTR;
		ty->ptr_to = calloc(1, sizeof(Type));
		ty->ptr_to->ty = TY_CHAR;
		node->type = ty;
		
		return node;
	}
	Token* tok = consume_ident();
	if (tok) {
		// 関数呼び出しかチェック
		if (consume("(")) {
			Node* node = calloc(1, sizeof(Node));
			node->kind = ND_CALL;
			
			// 関数名をコピー
			char* fname = calloc(tok->len + 1, sizeof(char));
			memcpy(fname, tok->str, tok->len);
			node->name = fname;
			
			// 引数の解析
			Node** args = malloc(sizeof(Node*) * MAX_FUNCTION_ARGS); // 最大10引数
			int argc = 0;
			if (!consume(")")) {
				do {
					args[argc++] = expr();
				} while (consume(","));
				expect(")");
			}
			args[argc] = NULL;
			node->args = args;
			node->argc = argc;
			
			return node;
		}
		
		// 変数参照 - ローカル変数を優先、次にグローバル変数
		Node* node = calloc(1, sizeof(Node));
		
		LVar* lvar = find_lvar(tok);
		if (lvar) {
			// ローカル変数が見つかった
			node->kind = ND_LVAR;
			node->offset = lvar->offset;
		} else {
			// ローカル変数にない場合、グローバル変数を探す
			GVar* gvar = find_gvar(tok);
			if (gvar) {
				node->kind = ND_GVAR;
				// グローバル変数名をコピー
				char* gname = calloc(tok->len + 1, sizeof(char));
				memcpy(gname, tok->str, tok->len);
				node->name = gname;
			} else {
				error("undefined variable");
			}
		}
		
		// 配列インデックス arr[i] を *(arr + i) に変換
		while (consume("[")) {
			Node* index = expr();
			expect("]");
			
			// arr[i] を *(arr + i) に変換
			Node* add_node = new_node(ND_ADD, node, index);
			node = new_node(ND_DEREF, add_node, NULL);
		}
		
		return node;
	}
	error("no match nodes");
}
// 単項演算子のノードを作成する関数
Node* unary() {
	// 前置インクリメント
	if (token->kind == TK_INC) {
		token = token->next;
		Node* node = calloc(1, sizeof(Node));
		node->kind = ND_PRE_INC;
		node->lhs = unary();
		return node;
	}
	// 前置デクリメント
	if (token->kind == TK_DEC) {
		token = token->next;
		Node* node = calloc(1, sizeof(Node));
		node->kind = ND_PRE_DEC;
		node->lhs = unary();
		return node;
	}
	if (consume("+"))
		return postfix();
	if (consume("-"))
		return new_node(ND_SUB, new_node_num(0), postfix());
	if (consume("&")) {
		Node* node = calloc(1, sizeof(Node));
		node->kind = ND_ADDR;
		node->lhs = unary();
		return node;
	}
	if (consume("*")) {
		Node* node = calloc(1, sizeof(Node));
		node->kind = ND_DEREF;
		node->lhs = unary();
		return node;
	}
	if (token->kind == TK_NOT) {
		fprintf(stderr, "DEBUG: Parsing NOT operator\n");
		token = token->next;
		Node* node = calloc(1, sizeof(Node));
		node->kind = ND_NOT;
		fprintf(stderr, "DEBUG: About to parse NOT operand\n");
		node->lhs = primary();  // 一時的にprimary()を使用してテスト
		fprintf(stderr, "DEBUG: Finished parsing NOT operand\n");
		return node;
	}
	return postfix();
}

// 後置演算子のノードを作成する関数
Node* postfix() {
	Node* node = primary();
	
	// 後置インクリメント
	if (token->kind == TK_INC) {
		token = token->next;
		Node* inc_node = calloc(1, sizeof(Node));
		inc_node->kind = ND_POST_INC;
		inc_node->lhs = node;
		return inc_node;
	}
	// 後置デクリメント
	if (token->kind == TK_DEC) {
		token = token->next;
		Node* dec_node = calloc(1, sizeof(Node));
		dec_node->kind = ND_POST_DEC;
		dec_node->lhs = node;
		return dec_node;
	}
	
	return node;
}