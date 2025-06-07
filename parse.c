#include "mipsc.h"

Node* code[100];
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
		// 数値
		if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p, 0);
			char* q = p;
			cur->val = strtol(p, &p, 10);
			cur->len = p - q;
			continue;
		}
		//二文字の記号
		if (startwith(p, "==") || startwith(p, "!=") || startwith(p, "<=") || startwith(p, ">=")) {
			cur = new_token(TK_RESERVED, cur, p, 2);
			p += 2;
			continue;
		}
		//一文字の記号
		if (strchr("+-*/()<>;={},", *p)) {
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
			if (len == 6 && !memcmp(start, "return", 6)) {
				cur = new_token(TK_RETURN, cur, start, len);
			} else if (len == 2 && !memcmp(start, "if", 2)) {
				cur = new_token(TK_IF, cur, start, len);
			} else if (len == 5 && !memcmp(start, "while", 5)) {
				cur = new_token(TK_WHILE, cur, start, len);
			} else if (len == 3 && !memcmp(start, "for", 3)) {
				cur = new_token(TK_FOR, cur, start, len);
			} else if (len == 3 && !memcmp(start, "int", 3)) {
				cur = new_token(TK_INT, cur, start, len);
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
	Node* node = equality();
	if (consume("="))
		node = new_node(ND_ASSIGN, node, assign());
	return node;
}
Node* expr() {
	return assign();
}
Node* stmt() {
	Node* node;
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
		Node** stmts = malloc(sizeof(Node*) * 100); // 最大100文
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
	// int functionname(params) { ... } の形式をパース
	if (!consume_int()) {
		error("function must start with 'int'");
	}
	
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
			if (!consume_int()) {
				error("parameter must be int");
			}
			Token* param_tok = consume_ident();
			if (!param_tok) {
				error("parameter name expected");
			}
			
			// パラメータをローカル変数リストに追加
			LVar* param = calloc(1, sizeof(LVar));
			param->next = params;
			param->name = param_tok->str;
			param->len = param_tok->len;
			param->offset = 8 + argc * 4; // 引数は$fp+8から
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
	Node** stmts = malloc(sizeof(Node*) * 100); // 最大100文
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
		code[i++] = function();
	}
	code[i] = NULL;
}
// 等しいと不等しいのノードを作成する関数
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
	if (consume("(")) {
		Node* node = expr();
		expect(")");
		return node;
	}
	if (token->kind == TK_NUM) {
		return new_node_num(expect_number());
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
			Node** args = malloc(sizeof(Node*) * 10); // 最大10引数
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
		
		// 変数参照
		Node* node = calloc(1, sizeof(Node));
		node->kind = ND_LVAR;
		
		LVar* lvar = find_lvar(tok);
		if (lvar) {
			node->offset = lvar->offset;
		} else {
			lvar = calloc(1, sizeof(LVar));
			lvar->next = locals;
			lvar->name = tok->str;
			lvar->len = tok->len;
			// ローカル変数は負のオフセット（引数と区別するため）
			int min_offset = 0;
			for (LVar* v = locals; v; v = v->next) {
				if (v->offset < min_offset) {
					min_offset = v->offset;
				}
			}
			lvar->offset = min_offset - 4;
			node->offset = lvar->offset;
			locals = lvar;
		}
		return node;
	}
	error("no match nodes");
}
// 単項演算子のノードを作成する関数
Node* unary() {
	if (consume("+"))
		return primary();
	if (consume("-"))
		return new_node(ND_SUB, new_node_num(0), primary());
	return primary();
}