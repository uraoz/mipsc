#define MIPSC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>


void error(char* fmt, ...);
void error_at(char* loc, char* fmt, ...);

//�g�[�N���̎�ނ�\��enum
typedef enum {
	TK_RESERVED, //�L��
	TK_NUM, //����
	TK_EOF, //���͂̏I���
} TokenKind;

typedef struct Token Token;

struct Token {
	TokenKind kind;
	Token* next; //���̃g�[�N��
	int val; //kind��TK_NUM�̂Ƃ��̐��l
	char* str; //�g�[�N��������
	int len; //�g�[�N���̒���
};


//���ۍ\���؂̃m�[�h�̎�ނ�\��enum
typedef enum {
	ND_ADD, //���Z
	ND_SUB, //���Z
	ND_MUL, //��Z
	ND_DIV, //���Z
	ND_NUM, //����
	ND_EQ, //������
	ND_NE, //�������Ȃ�
	ND_LT, //������
	ND_LE, //��������������
} NodeKind;

typedef struct Node Node;

//���ۍ\���؂̃m�[�h�̍\����
struct Node {
	NodeKind kind; //�m�[�h�̎��
	Node* lhs; //����
	Node* rhs; //�E��
	int val; //kind��ND_NUM�̂Ƃ��̐��l
};

extern char* user_input; //���͕�����
extern Token* token;//���ǂ�ł���token

// �p�[�T�֘A�̊֐�
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

// �R�[�h�����֘A�̊֐�
void gen(Node* node);

