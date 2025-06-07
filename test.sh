#\!/bin/bash
assert(){
	expected="$1"
	input="$2"

	./mipsc "$input" >tmp.s
	mips-linux-gnu-gcc -mno-abicalls -fno-pic tmp.s -o tmp -nostdlib -static
	qemu-mips tmp
	actual="$?"

	if [ "$actual" = "$expected" ]; then
		echo "$input => $actual"
	else
		echo "$input => $expected expected, but got $actual"
		exit 1
	fi
}

# 基本テスト
assert 0 "int main() { return 0; }"
assert 42 "int main() { return 42; }"
assert 6 "int main() { return 4+2; }"

# 変数宣言と代入のテスト
assert 1 "int main() { int a; a = 1; return a; }"
assert 42 "int main() { int x; x = 42; return x; }"
assert 2 "int main() { int a; int b; a = 1; b = 2; return b; }"
assert 1 "int main() { int a; int b; a = 1; b = 2; return a; }"
assert 3 "int main() { int a; int b; a = 1; b = 2; return a + b; }"

# 関数呼び出しのテスト
assert 7 "int plus(int x, int y) { return x + y; } int main() { return plus(3, 4); }"
assert 15 "int mul(int x, int y) { return x * y; } int main() { return mul(3, 5); }"

# 関数内でのローカル変数テスト
assert 10 "int test(int x) { int y; y = x + 5; return y; } int main() { return test(5); }"
assert 20 "int calc(int a, int b) { int c; int d; c = a * 2; d = b * 2; return c + d; } int main() { return calc(3, 7); }"

# sizeof()のテスト - 型サイズ
assert 4 "int main() { return sizeof(int); }"
assert 1 "int main() { return sizeof(char); }"
assert 4 "int main() { return sizeof(int*); }"
assert 4 "int main() { return sizeof(char*); }"
assert 4 "int main() { return sizeof(int**); }"

# sizeof()のテスト - 変数サイズ
assert 4 "int main() { int x; return sizeof(x); }"
assert 1 "int main() { char c; return sizeof(c); }"
assert 4 "int main() { int* p; return sizeof(p); }"
assert 4 "int main() { char* s; return sizeof(s); }"

# sizeof()のテスト - 式のサイズ
assert 4 "int main() { int a; int b; return sizeof(a + b); }"
assert 4 "int main() { int x; return sizeof(x * 2); }"
assert 4 "int main() { char c; int* p; return sizeof(&c); }"
assert 1 "int main() { char* p; return sizeof(*p); }"

# sizeof()のテスト - 配列実装準備
assert 4 "int main() { return sizeof(int) * 1; }"
assert 8 "int main() { return sizeof(int) * 2; }"
assert 40 "int main() { return sizeof(int) * 10; }"

# 配列のテスト - 宣言と基本的なアクセス
assert 1 "int main() { int a[2]; *a = 1; return *a; }"
assert 2 "int main() { int a[2]; *(a + 1) = 2; return *(a + 1); }"
assert 3 "int main() { int a[2]; *a = 1; *(a + 1) = 2; return *a + *(a + 1); }"

# 配列とポインタの相互運用
assert 3 "int main() { int a[2]; *a = 1; *(a + 1) = 2; int *p; p = a; return *p + *(p + 1); }"

# 配列のsizeof()テスト
assert 20 "int main() { int arr[5]; return sizeof(arr); }"
assert 12 "int main() { int arr[3]; return sizeof(arr); }"
assert 4 "int main() { char arr[4]; return sizeof(arr); }"

# 配列インデックス記法のテスト
assert 42 "int main() { int arr[3]; arr[0] = 42; return arr[0]; }"
assert 10 "int main() { int arr[5]; arr[0] = 10; arr[1] = 20; return arr[0]; }"
assert 20 "int main() { int arr[5]; arr[0] = 10; arr[1] = 20; return arr[1]; }"
assert 40 "int main() { int arr[5]; arr[0] = 10; arr[1] = 20; arr[2] = 30; return arr[0] + arr[2]; }"

# 配列名のポインタ変換テスト
assert 4 "int main() { int arr[5]; int *p; p = arr; return sizeof(p); }"
assert 42 "int main() { int arr[3]; int *p; p = arr; *p = 42; return arr[0]; }"

# 複雑な配列操作テスト
assert 55 "int main() { int arr[5]; arr[0] = 10; arr[1] = 15; arr[2] = 30; return arr[0] + arr[1] + arr[2]; }"
assert 21 "int main() { int arr[4]; arr[0] = 1; arr[1] = 2; arr[2] = 3; arr[3] = 15; return arr[0] + arr[1] + arr[2] + arr[3]; }"

# グローバル変数のテスト
assert 4 "int g; int main() { return sizeof(g); }"
assert 42 "int g; int main() { g = 42; return g; }"
assert 10 "int x; int y; int main() { x = 3; y = 7; return x + y; }"

# グローバル変数のシャドウイングテスト（ローカル変数優先）
assert 99 "int g; int main() { int g; g = 99; return g; }"
assert 123 "int x; int main() { x = 100; int x; x = 123; return x; }"

# グローバル配列のテスト
assert 12 "int arr[3]; int main() { return sizeof(arr); }"
assert 77 "int arr[3]; int main() { arr[1] = 77; return arr[1]; }"
assert 15 "int nums[4]; int main() { nums[0] = 5; nums[2] = 10; return nums[0] + nums[2]; }"

# グローバル変数と関数の組み合わせテスト
assert 42 "int foo; int bar() { return foo + 10; } int main() { foo = 32; return bar(); }"
assert 7 "int a; int b; int add() { return a + b; } int main() { a = 3; b = 4; return add(); }"

echo OK