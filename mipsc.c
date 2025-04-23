#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

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
Token* new_token(TokenKind kind, Token *cur, char *str) {
	Token* tok = calloc(1, sizeof(Token));
	tok->kind = kind;
	tok->str = str;
	cur->next = tok;
	return tok;
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
		if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p);
			cur->val = strtol(p, &p, 10);
			continue;
		}
		if (*p == '+' || *p == '-') {
			cur = new_token(TK_RESERVED, cur, p);
			p++;
			continue;
		}
		error("トークナイズできません");
	}
	new_token(TK_EOF, cur, p);
	return head.next;
}


int main(int argc, char **argv){
	if (argc != 2){
		fprintf(stderr,"引数が正しくない\n");
		return 1;
	}

	token = tokenize(argv[1]);


	//アセンブリの前半を出力
	printf(".text\n");
	printf(".globl main\n");
	printf(".globl __start\n");
	printf("__start:\n");
	printf("main:\n");

	//式の最初の値を読み込む
	printf("	li $a0, %d\n", expect_number());

	//'+値'または'-値'が続く限り読み込みアセンブリを出力
	while (!at_eof()) {
		if (consume('+')) {
			printf("	addi $a0, %d\n", expect_number());
			continue;
		}
		if (consume('-')) {
			printf("	addi $a0, -%d\n", expect_number());
			continue;
		}
		error("予期しないトークンです");
	}
	printf("	li $v0,4001\n");
	printf("	syscall\n");
	return 0;
}