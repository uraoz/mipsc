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
	TK_CHAR, // char
	TK_SIZEOF, // sizeof
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
	ND_GVAR, // グローバル変数
	ND_RETURN, // return文
	ND_IF, // if文
	ND_WHILE, // while文
	ND_FOR, // for文
	ND_BLOCK, // ブロック文
	ND_FUNC, // 関数定義
	ND_CALL, // 関数呼び出し
	ND_ADDR, // アドレス演算子 &
	ND_DEREF, // 間接参照演算子 *
	ND_SIZEOF, // sizeof演算子
} NodeKind;

// 型の種類を表すenum
typedef enum {
	TY_INT,   // int型
	TY_PTR,   // ポインタ型
	TY_CHAR,  // char型
	TY_ARRAY, // 配列型
} TypeKind;

// 型を表す構造体
typedef struct Type Type;
struct Type {
	TypeKind ty;      // 型の種類
	Type* ptr_to;     // ポインタが指す型（ポインタ型の場合のみ）
	size_t array_size; // 配列のサイズ（配列型の場合のみ）
};

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
	Type* type; // ノードの型情報
};

// 変数を管理する構造体
typedef struct LVar LVar;
struct LVar {
	LVar* next; // 次の変数かNULL
	char* name; // 変数の名前
	int len;    // 名前の長さ
	int offset; // RBPからのオフセット
	Type* type; // 変数の型情報
};

// グローバル変数を管理する構造体
typedef struct GVar GVar;
struct GVar {
	GVar* next; // 次のグローバル変数かNULL
	char* name; // 変数の名前
	int len;    // 名前の長さ
	Type* type; // 変数の型情報
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
extern GVar* globals; // グローバル変数のリスト
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
bool consume_char();
bool consume_sizeof();
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
GVar* find_gvar(Token* tok);
Function* find_function(Token* tok);
Type* parse_type_prefix();

// 型管理関連の関数
Type* new_type(TypeKind ty);
Type* pointer_to(Type* base);
Type* array_to(Type* base, size_t size);
bool is_integer(Type* ty);
int size_of(Type* ty);
Type* parse_type();
Type* get_type(Node* node);

// コード生成関連の関数
void gen(Node* node);
void gen_lval(Node* node);
