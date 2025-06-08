#include "mipsc.h"

char* user_input; // 入力文字列
Token* token; // 現在読んでいるtoken
LVar* locals; // ローカル変数のリスト
GVar* globals; // グローバル変数のリスト
Function* functions; // 関数のリスト
char* current_func_name; // 現在コード生成中の関数名
int current_frame_size; // 現在の関数のフレームサイズ
int label_count = 0; // ラベル生成用のカウンタ
int string_count = 0; // 文字列ラベル生成用のカウンタ
StringLiteral* string_literals = NULL; // 文字列リテラルのリスト

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

// ファイルを読み込んで文字列として返す関数
char* read_file(char* path) {
	FILE* fp = fopen(path, "r");
	if (!fp) {
		error("cannot open %s", path);
	}
	
	// ファイルサイズを取得
	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	// メモリを確保してファイル内容を読み込み
	char* buf = calloc(1, size + 2);  // null終端 + 改行用
	fread(buf, size, 1, fp);
	
	// ファイルの最後に改行がない場合は追加
	if (size == 0 || buf[size - 1] != '\n') {
		buf[size++] = '\n';
	}
	buf[size] = '\0';
	
	fclose(fp);
	return buf;
}

int main(int argc, char** argv) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <input.c or \"source code\">\n", argv[0]);
		return 1;
	}
	
	// ファイルパスかソースコード文字列かを判定
	if (strchr(argv[1], ' ') || strchr(argv[1], '{') || strchr(argv[1], ';')) {
		// 空白や特定の文字が含まれていれば直接のソースコード
		user_input = argv[1];
	} else {
		// そうでなければファイルパスとして扱う
		user_input = read_file(argv[1]);
	}
	token = tokenize(user_input);
	locals = NULL;
	globals = NULL;
	functions = NULL;
	current_func_name = NULL;
	string_literals = NULL;
	program();

	// アセンブリの前半部を出力
	printf(".data\n");
	printf("stack: .space 4096\n");
	
	// グローバル変数を出力
	for (GVar* var = globals; var; var = var->next) {
		printf("%s: .space %d\n", var->name, size_of(var->type));
	}
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
	
	// 文字列リテラルを出力（後から追加）
	if (string_literals) {
		printf("\n# String literals\n");
		for (StringLiteral* str = string_literals; str; str = str->next) {
			printf(".L_str_%d:\n", str->id);
			printf("	.asciiz \"");
			for (int i = 0; i < str->len; i++) {
				char c = str->data[i];
				switch (c) {
				case '\n': printf("\\n"); break;
				case '\t': printf("\\t"); break;
				case '\r': printf("\\r"); break;
				case '\\': printf("\\\\"); break;
				case '"': printf("\\\""); break;
				case '\0': printf("\\0"); break;
				default: printf("%c", c); break;
				}
			}
			printf("\"\n");
		}
	}
	
	return 0;
}