#ifndef MIPS_H
#define MIPS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

// トークンの種類を表すenum
typedef enum {
    TK_RESERVED, // 記号
    TK_NUM,      // 整数
    TK_EOF,      // 入力の終わり
} TokenKind;

// トークン型
typedef struct Token Token;
struct Token {
    TokenKind kind;  // トークンの種類
    Token *next;     // 次のトークン
    int val;         // kindがTK_NUMのときの数値
    char *str;       // トークン文字列
    int len;         // トークンの長さ
};

// 抽象構文木のノードの種類を表すenum
typedef enum {
    ND_ADD, // 加算
    ND_SUB, // 減算
    ND_MUL, // 乗算
    ND_DIV, // 除算
    ND_NUM, // 整数
    ND_EQ,  // 等しい
    ND_NE,  // 等しくない
    ND_LT,  // 小さい
    ND_LE,  // 小さいか等しい
} NodeKind;

// 抽象構文木のノードの構造体
typedef struct Node Node;
struct Node {
    NodeKind kind; // ノードの種類
    Node* lhs;     // 左辺
    Node* rhs;     // 右辺
    int val;       // kindがND_NUMのときの数値
};

// グローバル変数
extern char *user_input; // 入力文字列
extern Token *token;     // 現在のトークン

// エラー関連の関数
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);

// パーサ関連の関数
Token* tokenize(char* p);
bool consume(char* op);
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

Node* new_node(NodeKind kind, Node* lhs, Node* rhs);
Node* new_node_num(int val);

// コード生成関連の関数
void gen(Node* node);

#endif // MIPS_H