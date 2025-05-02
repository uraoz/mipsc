#include "mipsc.h"

char* user_input; //入力文字列
Token* token; //今読んでいるtoken

void error(char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}
//エラー箇所を報告する
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
	//トークナイズ
	user_input = argv[1];
	token = tokenize(user_input);
	Token* tmp = token;
	printf("デバッグ: トークン一覧\n");
	while (tmp) {
		if (tmp->kind == TK_RESERVED) {
			printf("予約語: %.*s\n", tmp->len, tmp->str);
		}
		else if (tmp->kind == TK_NUM) {
			printf("数値: %d\n", tmp->val);
		}
		else if (tmp->kind == TK_EOF) {
			printf("EOF\n");
		}
		tmp = tmp->next;
	}
	program();

	//アセンブリの前半を出力
	printf(".data\n");
	printf("stack: .space 4096\n");
	printf(".text\n");
	printf(".globl main\n");
	printf(".globl __start\n");
	printf("__start:\n");
	printf("main:\n");
	//変数の領域の確保
	printf("	sw $t0, 0($fp)\n");
	printf("	addi $fp, $fp, -4\n");
	printf("	addi $fp, $sp, 0\n");
	printf("	addi $sp, $sp, -4096\n");
	//code[0]から順に命令を出力
	for (int i = 0; code[i]; i++) {
		gen(code[i]);
		//式の結果にスタックが残っているのでpopする
		printf("	lw $t1, 0($sp)\n");
		printf("	addi $sp, $sp, 4\n");
	}
	//終了処理　$t1に結果が入っているので返り値に入れる
	printf("	addi $sp, $fp, 0\n");
	printf("	addi $fp, $fp, 4\n");
	printf("	lw $t0, $sp, 4\n");
	printf("	addi $sp, $sp, 4\n");
	printf("	addi $a0, $t1, 0\n");//実行コード
	printf("	li $v0,4001\n");
	printf("	syscall\n");
	return 0;
}