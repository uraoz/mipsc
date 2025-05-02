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
		fprintf(stderr, "not correct arguments\n");
		return 1;
	}
	//�g�[�N�i�C�Y
	user_input = argv[1];
	token = tokenize(user_input);
	Token* tmp = token;
	printf("�f�o�b�O: �g�[�N���ꗗ\n");
	while (tmp) {
		if (tmp->kind == TK_RESERVED) {
			printf("�\���: %.*s\n", tmp->len, tmp->str);
		}
		else if (tmp->kind == TK_NUM) {
			printf("���l: %d\n", tmp->val);
		}
		else if (tmp->kind == TK_EOF) {
			printf("EOF\n");
		}
		tmp = tmp->next;
	}
	program();

	//�A�Z���u���̑O�����o��
	printf(".data\n");
	printf("stack: .space 4096\n");
	printf(".text\n");
	printf(".globl main\n");
	printf(".globl __start\n");
	printf("__start:\n");
	printf("main:\n");
	//�ϐ��̗̈�̊m��
	printf("	sw $t0, 0($fp)\n");
	printf("	addi $fp, $fp, -4\n");
	printf("	addi $fp, $sp, 0\n");
	printf("	addi $sp, $sp, -4096\n");
	//code[0]���珇�ɖ��߂��o��
	for (int i = 0; code[i]; i++) {
		gen(code[i]);
		//���̌��ʂɃX�^�b�N���c���Ă���̂�pop����
		printf("	lw $t1, 0($sp)\n");
		printf("	addi $sp, $sp, 4\n");
	}
	//�I�������@$t1�Ɍ��ʂ������Ă���̂ŕԂ�l�ɓ����
	printf("	addi $sp, $fp, 0\n");
	printf("	addi $fp, $fp, 4\n");
	printf("	lw $t0, $sp, 4\n");
	printf("	addi $sp, $sp, 4\n");
	printf("	addi $a0, $t1, 0\n");//���s�R�[�h
	printf("	li $v0,4001\n");
	printf("	syscall\n");
	return 0;
}