#include "mipsc.h"

char* user_input; //���͕�����
Token* token; //���ǂ�ł���token

void error(char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}
//�G���[�ӏ���񍐂���
void error_at(char* loc, char* fmt, ...) {
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

int main(int argc, char** argv) {
	if (argc != 2) {
		fprintf(stderr, "�������������Ȃ�\n");
		return 1;
	}
	user_input = argv[1];
	token = tokenize(user_input);
	Node* node = expr();

	//�A�Z���u���̑O�����o��
	printf(".data\n");
	printf("stack: .space 4096\n");
	printf(".text\n");
	printf(".globl main\n");
	printf(".globl __start\n");
	printf("__start:\n");
	printf("main:\n");
	//�X�^�b�N�|�C���^��������
	gen(node);
	printf("	lw $t0, 0($sp)\n");
	printf("	addi $sp, $sp, 4\n");
	printf("	addi $a0, $t0, 0\n");//���s�R�[�h
	printf("	li $v0,4001\n");
	printf("	syscall\n");
	return 0;
}