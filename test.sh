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

echo OK