#define MIPSC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>


void error(char* fmt, ...);
void error_at(char* loc, char* fmt, ...);

// トークンの種類を表すenum
typedef enum {
	TK_RESERVED, // 記号
	TK_NUM, // 整数
	TK_IDENT, // 識別子
	TK_RETURN, // return
	TK_IF, // if
	TK_WHILE, // while
	TK_FOR, // for
	TK_INT, // int
	TK_EOF, // 入力の終わり
} TokenKind;

typedef struct Token Token;

struct Token {
	TokenKind kind;
	Token* next; // 次のトークン
	int val; // kindがTK_NUMのときの数値
	char* str; // トークン文字列
	int len; // トークンの長さ
};


// 抽象構文木のノードの種類を表すenum
typedef enum {
	ND_ADD, // 加算
	ND_SUB, // 減算
	ND_MUL, // 乗算
	ND_DIV, // 除算
	ND_NUM, // 整数
	ND_EQ, // 等しい
	ND_NE, // 等しくない
	ND_LT, // 小さい
	ND_LE, // 小さいか等しい
	ND_ASSIGN, // 代入
	ND_LVAR, // ローカル変数
	ND_RETURN, // return文
	ND_IF, // if文
	ND_WHILE, // while文
	ND_FOR, // for文
	ND_BLOCK, // ブロック文
	ND_FUNC, // 関数定義
	ND_CALL, // 関数呼び出し
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの構造体
struct Node {
	NodeKind kind; // ノードの種類
	Node* lhs; // 左辺
	Node* rhs; // 右辺
	Node* cond; // if文/while文/for文の条件
	Node* then; // if文/while文/for文のthen/body節
	Node* init; // for文の初期化
	Node* inc; // for文のインクリメント
	Node** body; // ブロック文の本体（文のリスト）
	Node** args; // 関数呼び出しの引数リスト
	char* name; // 関数名
	int val; // kindがND_NUMのときの数値
	int offset; // kindがND_LVARのときのオフセット
	int argc; // 引数の数
};

// 変数を管理する構造体
typedef struct LVar LVar;
struct LVar {
	LVar* next; // 次の変数かNULL
	char* name; // 変数の名前
	int len;    // 名前の長さ
	int offset; // RBPからのオフセット
};

// 関数を管理する構造体
typedef struct Function Function;
struct Function {
	Function* next; // 次の関数かNULL
	char* name; // 関数名
	int len; // 名前の長さ
	Node* node; // 関数定義のノード
	LVar* locals; // その関数のローカル変数
};

extern char* user_input; // 入力文字列
extern Token* token; // 現在読んでいるtoken
extern LVar* locals; // ローカル変数のリスト
extern Function* functions; // 関数のリスト
extern char* current_func_name; // 現在コード生成中の関数名
extern int current_frame_size; // 現在の関数のフレームサイズ
extern int label_count; // ラベル生成用のカウンタ

// パーサ関連の関数
Token* tokenize(char* p);
Token* consume_ident();
bool consume(char* op);
bool consume_return();
bool consume_if();
bool consume_while();
bool consume_for();
bool consume_int();
void expect(char* op);
int expect_number();
bool at_eof();

Node* expr();
Node* equality();
Node* relational();
Node* add();
Node* mul();
Node* unary();
Node* primary();
void program();
extern Node* code[100];
Node* new_node(NodeKind kind, Node* lhs, Node* rhs);
Node* new_node_num(int val);
LVar* find_lvar(Token* tok);
Function* find_function(Token* tok);

// コード生成関連の関数
void gen(Node* node);
void gen_lval(Node* node);
