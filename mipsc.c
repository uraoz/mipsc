#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>


char *user_input; //入力文字列

//エラー箇所を報告する
void error_at(char *loc, char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int pos = loc - user_input;
	fprintf(stderr, "%s\n", user_input);
	fprintf(stderr, "%*s", pos, " ");
	fprintf(stderr, "^ ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}




//トークンの種類を表すenum
typedef enum{
	TK_RESERVED, //記号
	TK_NUM, //整数
	TK_EOF, //入力の終わり
} TokenKind;

typedef struct Token Token;

struct Token{
	TokenKind kind;
	Token *next; //次のトークン
	int val; //kindがTK_NUMのときの数値
	char *str; //トークン文字列
};


//抽象構文木のノードの種類を表すenum
typedef enum {
	ND_ADD, //加算
	ND_SUB, //減算
	ND_MUL, //乗算
	ND_DIV, //除算
	ND_NUM, //整数
} NodeKind;

typedef struct Node Node;

//抽象構文木のノードの構造体
struct Node {
	NodeKind kind; //ノードの種類
	Node* lhs; //左辺
	Node* rhs; //右辺
	int val; //kindがND_NUMのときの数値
};

// 関数プロトタイプ宣言
bool consume(char op);
void expect(char op);
int expect_number();
void error(char* fmt, ...);
Token* tokenize(char* p);
Token* new_token(TokenKind kind, Token* cur, char* str);
Node* new_node(NodeKind kind, Node* lhs, Node* rhs);
Node* new_node_num(int val);
Node* expr();
Node* mul();
Node* primary();
Node* unary();
void gen(Node* node);


Token *token;//今読んでいるtoken

void error(char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}
// 次のトークンを読み込んで期待したものか返す
bool consume(char op) {
	if (token->kind != TK_RESERVED || token->str[0] != op)
		return false;
	token = token->next;
	return true;
}
// t次のトークンが期待したものならトークンを一つ読み進める
// それ以外ではエラーを返す
void expect(char op) {
	if (token->kind != TK_RESERVED || token->str[0] != op)
		error("'%c'ではありません", op);
	token = token->next;
}

// 次のトークンが数値ならトークンを一つ読み進めてその数値を返す
int expect_number() {
	if (token->kind != TK_NUM)
		error("数ではありません");
	int val = token->val;
	token = token->next;
	return val;
}

bool at_eof() {
	return token->kind == TK_EOF;
}

//新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str) {
	Token *tok = calloc(1, sizeof(Token));
	tok->kind = kind;
	tok->str = str;
	cur->next = tok;
	return tok;
}
// 入力文字列pをトークンに分割してそれを返す
Token *tokenize(char *p) {
	Token head;
	head.next = NULL;
	Token *cur = &head;
	while (*p) {
		if (isspace(*p)) {
			p++;
			continue;
		}
		if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p);
			cur->val = strtol(p, &p, 10);
			continue;
		}
		if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || * p == ')') {
			cur = new_token(TK_RESERVED, cur, p);
			p++;
			continue;
		}
		error("トークナイズできません");
	}
	new_token(TK_EOF, cur, p);
	return head.next;
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
Node* expr() {
	Node* node = mul();
	while (1) {
		if (consume('+')) {
			node = new_node(ND_ADD, node, mul());
			continue;
		}
		if (consume('-')) {
			node = new_node(ND_SUB, node, mul());
			continue;
		}
		break;
	}
	return node;
}

Node* mul() {
	Node* node = unary();
	while (1) {
		if (consume('*')) {
			node = new_node(ND_MUL, node, unary());
			continue;
		}
		if (consume('/')) {
			node = new_node(ND_DIV, node, unary());
			continue;
		}
		break;
	}
	return node;
}
Node* primary() {
	if (consume('(')) {
		Node* node = expr();
		expect(')');
		return node;
	}
	if (token->kind == TK_NUM) {
		return new_node_num(expect_number());
	}
	error("数値でも'('でもありません");
}

Node* unary() {
	if (consume('+'))
		return primary();
	if (consume('-'))
		return new_node(ND_SUB, new_node_num(0), primary());
	return primary();
}
//スタックマシンっぽい実装
void gen(Node *node) {
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
	default:
		error("不正な演算子です");
	}
	
	printf("	addi $sp,$sp, -4\n");
	printf("	sw $t0, 0($sp)\n");
}

int main(int argc, char **argv){
	if (argc != 2){
		fprintf(stderr,"引数が正しくない\n");
		return 1;
	}
	user_input = argv[1];
	token = tokenize(user_input);
	Node* node = expr();

	//アセンブリの前半を出力
	printf(".data\n");
	printf("stack: .space 4096\n");
	printf(".text\n");
	printf(".globl main\n");
	printf(".globl __start\n");
	printf("__start:\n");
	printf("main:\n");
	//スタックポインタを初期化
	gen(node);
	printf("	lw $t0, 0($sp)\n");
	printf("	addi $sp, $sp, 4\n");
	printf("	addi $a0, $t0, 0\n");//実行コード
	printf("	li $v0,4001\n");
	printf("	syscall\n");
	return 0;
}