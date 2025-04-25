#include "mipsc.h"

// ���̃g�[�N����ǂݍ���Ŋ��҂������̂��Ԃ�
bool consume(char* op) {
	if (token->kind != TK_RESERVED || token->len != strlen(op) || memcmp(token->str, op, token->len))
		return false;
	token = token->next;
	return true;
}
// t���̃g�[�N�������҂������̂Ȃ�g�[�N������ǂݐi�߂�
// ����ȊO�ł̓G���[��Ԃ�
void expect(char* op) {
	if (token->kind != TK_RESERVED || token->len != strlen(op) || memcmp(token->str, op, token->len))
		error("'%c'�ł͂���܂���", op);
	token = token->next;
}

// ���̃g�[�N�������l�Ȃ�g�[�N������ǂݐi�߂Ă��̐��l��Ԃ�
int expect_number() {
	if (token->kind != TK_NUM)
		error("���ł͂���܂���");
	int val = token->val;
	token = token->next;
	return val;
}

bool at_eof() {
	return token->kind == TK_EOF;
}

//�V�����g�[�N�����쐬����cur�Ɍq����
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
// ���͕�����p���g�[�N���ɕ������Ă����Ԃ�
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
			cur = new_token(TK_NUM, cur, p, 0);
			char* q = p;
			cur->val = strtol(p, &p, 10);
			cur->len = p - q;
			continue;
		}
		//�ꕶ���̋L��
		if (strchr("+-*/()<>", *p)) {
			cur = new_token(TK_RESERVED, cur, p, 1);
			p++;
			continue;
		}
		//�񕶎��̋L��
		if (startwith(p, "==") || startwith(p, "!=") || startwith(p, "<=") || startwith(p, ">=")) {
			cur = new_token(TK_RESERVED, cur, p, 2);
			p += 2;
			continue;
		}

	}
	new_token(TK_EOF, cur, p, 0);
	return head.next;
}


//�V�����m�[�h���쐬����֐�
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

//�m�[�h�̃p�[�T
Node* expr() {
	return equality();
}
//�����ƕs�����̃m�[�h���쐬����֐�
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
//���Ȃ�Ƒ�Ȃ�̃m�[�h���쐬����֐�
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
//���Z�ƌ��Z�̃m�[�h���쐬����֐�
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
//��Z�Ə��Z�̃m�[�h���쐬����֐�
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
//���l�܂���'('�Ŏn�܂�m�[�h���쐬����֐�
Node* primary() {
	if (consume("(")) {
		Node* node = expr();
		expect(")");
		return node;
	}
	if (token->kind == TK_NUM) {
		return new_node_num(expect_number());
	}
	error("���l�ł�'('�ł�����܂���");
}
//�P�����Z�q�̃m�[�h���쐬����֐�
Node* unary() {
	if (consume("+"))
		return primary();
	if (consume("-"))
		return new_node(ND_SUB, new_node_num(0), primary());
	return primary();
}