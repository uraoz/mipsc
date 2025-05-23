#include "mipsc.h"

Node* code[100];
// 次のトークンを読み込んで期待したものか返す
bool consume(char* op) {
	if (token->kind != TK_RESERVED || token->len != strlen(op) || memcmp(token->str, op, token->len))
		return false;
	token = token->next;
	return true;
}
// t次のトークンが期待したものならトークンを一つ読み進める
// それ以外ではエラーを返す
void expect(char* op) {
	if (token->kind != TK_RESERVED || token->len != strlen(op) || memcmp(token->str, op, token->len))
		error("'%s'is expected but not", op);
	token = token->next;
}

// 次のトークンが数値ならトークンを一つ読み進めてその数値を返す
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

//新しいトークンを作成してcurに繋げる
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
// 入力文字列pをトークンに分割してそれを返す
Token* tokenize(char* p) {
	Token head;
	head.next = NULL;
	Token* cur = &head;
	while (*p) {
		if (isspace(*p)) {
			p++;
			continue;
		}
		//数値
		if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p, 0);
			char* q = p;
			cur->val = strtol(p, &p, 10);
			cur->len = p - q;
			continue;
		}
		//一文字の記号
		if (strchr("+-*/()<>;", *p)) {
			cur = new_token(TK_RESERVED, cur, p, 1);
			p++;
			continue;
		}
		//二文字の記号
		if (startwith(p, "==") || startwith(p, "!=") || startwith(p, "<=") || startwith(p, ">=")) {
			cur = new_token(TK_RESERVED, cur, p, 2);
			p += 2;
			continue;
		}
		//1文字の変数
		if ('a' <= *p && *p <= 'z') {
			cur = new_token(TK_IDENT, cur, p, 1);
			cur->len = 1;
			p++;
			continue;
		}

	}
	new_token(TK_EOF, cur, p, 0);
	return head.next;
}
Token* consume_ident() {
	if (token->kind != TK_IDENT)
		return NULL;
	Token* tok = token;
	token = token->next;
	return tok;
}


//新しいノードを作成する関数
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


//ノードのパーサ
/*
program    = stmt*
stmt       = expr ";"
expr       = assign
assign     = equality ("=" assign)?
equality   = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add        = mul ("+" mul | "-" mul)*
mul        = unary ("*" unary | "/" unary)*
unary      = ("+" | "-")? primary
primary    = num | ident | "(" expr ")"
*/

//セミコロン区切りの式をパースする関数

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
	Node* node = expr();
	expect(";");
	return node;
}
void program() {
	int i = 0;
	while (!at_eof()) {
		code[i++] = stmt();
	}
	code[i] = NULL;
}
//等号と不等号のノードを作成する関数
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
//小なりと大なりのノードを作成する関数
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
//加算と減算のノードを作成する関数
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
//乗算と除算のノードを作成する関数
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
//数値または'('で始まるノードを作成する関数
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
		Node* node = calloc(1, sizeof(Node));
		node->kind = ND_LVAR;
		node->offset = (tok->str[0] - 'a' + 1) * 8;//変数名はaからz
		return node;
	}
	error("no mach nodes");
}
//単項演算子のノードを作成する関数
Node* unary() {
	if (consume("+"))
		return primary();
	if (consume("-"))
		return new_node(ND_SUB, new_node_num(0), primary());
	return primary();
}