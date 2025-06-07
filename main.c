#include "mipsc.h"

char* user_input; // 入力文字列
Token* token; // 現在読んでいるtoken
LVar* locals; // ローカル変数のリスト
Function* functions; // 関数のリスト
char* current_func_name; // 現在コード生成中の関数名
int current_frame_size; // 現在の関数のフレームサイズ
int label_count = 0; // ラベル生成用のカウンタ

void error(char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}
// エラー箇所を報告する
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
	user_input = argv[1];
	token = tokenize(user_input);
	locals = NULL;
	functions = NULL;
	current_func_name = NULL;
	program();

	// アセンブリの前半部を出力
	printf(".data\n");
	printf("stack: .space 4096\n");
	printf(".text\n");
	printf(".globl main\n");
	printf(".globl __start\n");
	printf("__start:\n");
	// スタックポインタの初期化
	printf("	addi $s8, $sp, 0\n");
	printf("	addi $sp, $sp, -4096\n");
	printf("	jal main\n");
	printf("	nop\n");
	printf("	move $a0, $v0\n");
	printf("	li $v0, 4001\n");
	printf("	syscall\n");
	
	//すべての関数を出力
	for (int i = 0; code[i]; i++) {
		gen(code[i]);
	}
	return 0;
}