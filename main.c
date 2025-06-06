#include "mipsc.h"

char* user_input; // 入力文字列
Token* token; // 現在読んでいるtoken
LVar* locals; // ローカル変数のリスト
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
	program();

	// アセンブリの前半部を出力
	printf(".data\n");
	printf("stack: .space 4096\n");
	printf(".text\n");
	printf(".globl main\n");
	printf(".globl __start\n");
	printf("__start:\n");
	printf("main:\n");
	//フレームポインタとスタックポインタの初期化
	printf("	addi $fp, $sp, 0\n");
	printf("	addi $sp, $sp, -4096\n");
	//code[0]から順に命令を出力
	for (int i = 0; code[i]; i++) {
		gen(code[i]);
		//式の結果にスタックに残っているのでpopする
		printf("	lw $t1, 0($sp)\n");
		printf("	addi $sp, $sp, 4\n");
	}
	//終了処理 $t1に結果が入っているので返り値に入力
	printf("	addi $a0, $t1, 0\n");//実行コード
	printf("	li $v0,4001\n");
	printf("	syscall\n");
	return 0;
}