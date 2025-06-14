#ifndef MIPSC_H
#define MIPSC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

// 定数定義
// 配列とバッファサイズの制限
#define MAX_STATEMENTS 100
#define MAX_FUNCTION_ARGS 10

// 型サイズ (MIPS32)
#define SIZE_INT 4
#define SIZE_PTR 4
#define SIZE_CHAR 1
#define MIN_ALIGNMENT 4

// スタックフレームオフセット
#define ARG_SAVE_OFFSET -8
#define ARG_SIZE 4

// キーワード長
#define LEN_RETURN 6
#define LEN_IF 2
#define LEN_ELSE 4
#define LEN_WHILE 5
#define LEN_FOR 3
#define LEN_VOID 4
#define LEN_INT 3
#define LEN_CHAR 4
#define LEN_SIZEOF 6
#define LEN_STRUCT 6
#define LEN_BREAK 5
#define LEN_CONTINUE 8

// コメント関連
#define COMMENT_LINE_START "//"
#define COMMENT_BLOCK_START "/*"
#define COMMENT_BLOCK_END "*/"

void error(char* fmt, ...);
void error_at(char* loc, char* fmt, ...);

// トークンの種類を表すenum
typedef enum {
	TK_RESERVED, // 記号
	TK_NUM, // 整数
	TK_IDENT, // 識別子
	TK_STR, // 文字列リテラル
	TK_CHAR_LITERAL, // 文字リテラル
	TK_RETURN, // return
	TK_IF, // if
	TK_ELSE, // else
	TK_WHILE, // while
	TK_FOR, // for
	TK_VOID, // void
	TK_INT, // int
	TK_CHAR, // char
	TK_SIZEOF, // sizeof
	TK_STRUCT, // struct
	TK_TYPEDEF, // typedef
	TK_ADD_ASSIGN, // +=
	TK_SUB_ASSIGN, // -=
	TK_MUL_ASSIGN, // *=
	TK_DIV_ASSIGN, // /=
	TK_INC, // ++
	TK_DEC, // --
	TK_ARROW, // ->
	TK_AND, // &&
	TK_OR, // ||
	TK_NOT, // !
	TK_QUESTION, // ?
	TK_COLON, // :
	TK_BREAK, // break
	TK_CONTINUE, // continue
	TK_EOF, // 入力の終わり
} TokenKind;

typedef struct Token Token;

struct Token {
	TokenKind kind;
	Token* next; // 次のトークン
	int val; // kindがTK_NUMのときの数値
	char* str; // トークン文字列
	int len; // トークンの長さ
	char* str_data; // kindがTK_STRのときのエスケープ処理済み文字列データ
	int str_len; // kindがTK_STRのときの処理済み文字列の長さ
};


// 抽象構文木のノードの種類を表すenum
typedef enum {
	ND_ADD, // 加算
	ND_SUB, // 減算
	ND_MUL, // 乗算
	ND_DIV, // 除算
	ND_MOD, // 剰余
	ND_NUM, // 整数
	ND_EQ, // 等しい
	ND_NE, // 等しくない
	ND_LT, // 小さい
	ND_LE, // 小さいか等しい
	ND_ASSIGN, // 代入
	ND_ADD_ASSIGN, // += 代入
	ND_SUB_ASSIGN, // -= 代入
	ND_MUL_ASSIGN, // *= 代入
	ND_DIV_ASSIGN, // /= 代入
	ND_PRE_INC, // 前置インクリメント ++x
	ND_POST_INC, // 後置インクリメント x++
	ND_PRE_DEC, // 前置デクリメント --x
	ND_POST_DEC, // 後置デクリメント x--
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
	ND_STR, // 文字列リテラル
	ND_AND, // 論理AND &&
	ND_OR, // 論理OR ||
	ND_NOT, // 論理NOT !
	ND_TERNARY, // 三項演算子 ? :
	ND_BREAK, // break文
	ND_CONTINUE, // continue文
	ND_MEMBER, // メンバアクセス (.)
	ND_STRUCT_DEF, // 構造体定義
	ND_BUILTIN_CALL, // 組み込み関数呼び出し
} NodeKind;

// 型の種類を表すenum
typedef enum {
	TY_VOID,   // void型
	TY_INT,    // int型
	TY_PTR,    // ポインタ型
	TY_CHAR,   // char型
	TY_ARRAY,  // 配列型
	TY_STRUCT, // 構造体型
} TypeKind;

// 組み込み関数の種類を表すenum
typedef enum {
	BUILTIN_PUTCHAR,  // int putchar(int c)
	BUILTIN_GETCHAR,  // int getchar(void)
	BUILTIN_PUTS,     // int puts(const char* s)
	BUILTIN_STRLEN,   // int strlen(const char* s)
	BUILTIN_STRCMP,   // int strcmp(const char* s1, const char* s2)
	BUILTIN_STRCPY,   // char* strcpy(char* dest, const char* src)
} BuiltinKind;

// 前方宣言
typedef struct Type Type;

// 構造体のメンバを表す構造体
typedef struct Member Member;
struct Member {
	Member* next;     // 次のメンバ
	char* name;       // メンバ名
	int name_len;     // メンバ名の長さ
	Type* type;       // メンバの型
	int offset;       // 構造体の先頭からのオフセット
};

// 構造体の定義を表す構造体
typedef struct StructDef StructDef;
struct StructDef {
	StructDef* next;  // 次の構造体定義
	char* name;       // 構造体名
	int name_len;     // 構造体名の長さ
	Member* members;  // メンバのリスト
	int size;         // 構造体全体のサイズ
};

// 型を表す構造体
typedef struct Type Type;
struct Type {
	TypeKind ty;       // 型の種類
	Type* ptr_to;      // ポインタが指す型（ポインタ型の場合のみ）
	size_t array_size; // 配列のサイズ（配列型の場合のみ）
	StructDef* struct_def; // 構造体定義（構造体型の場合のみ）
};

typedef struct Node Node;

// 抽象構文木のノードの構造体
struct Node {
	NodeKind kind; // ノードの種類
	Node* lhs; // 左辺
	Node* rhs; // 右辺
	Node* cond; // if文/while文/for文の条件
	Node* then; // if文/while文/for文のthen/body節
	Node* els; // if文のelse節
	Node* init; // for文の初期化
	Node* inc; // for文のインクリメント
	Node** body; // ブロック文の本体（文のリスト）
	Node** args; // 関数呼び出しの引数リスト
	char* name; // 関数名
	int val; // kindがND_NUMのときの数値
	int offset; // kindがND_LVARのときのオフセット
	int argc; // 引数の数
	Type* type; // ノードの型情報
	char* str; // kindがND_STRのときの文字列データ
	int str_len; // kindがND_STRのときの文字列の長さ
	BuiltinKind builtin_kind; // kindがND_BUILTIN_CALLのときの組み込み関数の種類
};

// 文字列リテラルを管理する構造体
typedef struct StringLiteral StringLiteral;
struct StringLiteral {
	StringLiteral* next; // 次の文字列リテラルかNULL
	char* data; // 文字列データ
	int len; // 文字列の長さ
	int id; // 文字列のID
};

// 変数を管理する構造体
typedef struct LVar LVar;
struct LVar {
	LVar* next; // 次の変数かNULL
	char* name; // 変数の名前
	int len;    // 名前の長さ
	int offset; // $s8からのオフセット
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

// ループラベル管理構造
typedef struct LoopLabel LoopLabel;
struct LoopLabel {
	int break_label;    // breakのジャンプ先ラベル
	int continue_label; // continueのジャンプ先ラベル
	LoopLabel* next;    // ネストしたループ用のスタック
};

extern char* user_input; // 入力文字列
extern Token* token; // 現在読んでいるtoken
extern LVar* locals; // ローカル変数のリスト
extern GVar* globals; // グローバル変数のリスト
extern Function* functions; // 関数のリスト
extern StructDef* struct_defs; // 構造体定義のリスト
extern char* current_func_name; // 現在コード生成中の関数名
extern int current_frame_size; // 現在の関数のフレームサイズ
extern int label_count; // ラベル生成用のカウンタ
extern int string_count; // 文字列ラベル生成用のカウンタ
extern StringLiteral* string_literals; // 文字列リテラルのリスト
extern LoopLabel* loop_stack; // ループラベルスタック

// パーサ関連の関数
Token* tokenize(char* p);
Token* consume_ident();
bool consume(char* op);
bool consume_return();
bool consume_break();
bool consume_continue();
bool consume_if();
bool consume_while();
bool consume_for();
bool consume_void();
bool consume_int();
bool consume_char();
bool consume_sizeof();
bool consume_struct();
bool consume_typedef();
void expect(char* op);
int expect_number();
bool at_eof();

Node* expr();
Node* ternary();
Node* logical_or();
Node* logical_and();
Node* equality();
Node* relational();
Node* add();
Node* mul();
Node* unary();
Node* primary();
Node* postfix();
void program();
extern Node* code[MAX_STATEMENTS];
Node* new_node(NodeKind kind, Node* lhs, Node* rhs);
Node* new_node_num(int val);
LVar* find_lvar(Token* tok);
GVar* find_gvar(Token* tok);
Function* find_function(Token* tok);
StructDef* find_struct(Token* tok);
Member* find_member(StructDef* struct_def, Token* tok);
Type* parse_type_prefix();

// 型管理関連の関数
Type* new_type(TypeKind ty);
Type* pointer_to(Type* base);
Type* array_to(Type* base, size_t size);
Type* struct_to(StructDef* struct_def);
bool is_integer(Type* ty);
int size_of(Type* ty);
Type* parse_type();
Type* get_type(Node* node);

// コード生成関連の関数
void gen(Node* node);
void gen_lval(Node* node);
void gen_compound_assign(Node* node, const char* operation, bool is_div);
void gen_inc_dec(Node* node, int delta, bool is_prefix);
void gen_variable_access(Node* node, bool is_local);

// printf実装の補助関数
void gen_printf_call(Node* node);
void gen_printf_char(int printf_id);
void gen_printf_integer(int printf_id, int arg_index);
void gen_write_syscall(void);

// 組み込み関数サポート
BuiltinKind get_builtin_kind(Token* tok);
void gen_builtin_call(Node* node);
void gen_putchar(Node* node);
void gen_getchar(Node* node);
void gen_puts(Node* node);
void gen_strlen(Node* node);
void gen_strcmp(Node* node);
void gen_strcpy(Node* node);

// ループラベル管理関数
void push_loop_labels(int break_label, int continue_label);
void pop_loop_labels();

#endif // MIPSC_H
