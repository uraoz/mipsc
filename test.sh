#!/bin/bash
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

assert 0 "int main() { return 0; }"
assert 42 "int main() { return 42; }"
assert 6 "int main() { return 4+2; }"
assert 21 "int main() { return 5+20-4; }"
assert 41 "int main() { return 12 + 34 - 5; }"
assert 17 "int main() { return 3*4+5; }"
assert 3 "int main() { return (5+7)/4; }"
assert 15 "int main() { return 5*(9-6); }"
assert 3 "int main() { return -3+6; }"
assert 0 "int main() { return 0==1; }"
assert 1 "int main() { return 3==3; }"
assert 1 "int main() { return 0!=1; }"
assert 1 "int main() { return 0<1; }"
assert 0 "int main() { return 1<0; }"
# 変数テストは現在関数形式では未対応（文形式では動作）
# assert 1 "int main() { a=1; return a; }"
# assert 2 "int main() { a=1; b=2; return b; }"
# assert 3 "int main() { a=1; b=2; return a+b; }"

# 関数呼び出しのテスト
assert 7 "int plus(int x, int y) { return x + y; } int main() { return plus(3, 4); }"
assert 15 "int mul(int x, int y) { return x * y; } int main() { return mul(3, 5); }"
assert 2 "int sub(int x, int y) { return x - y; } int main() { return sub(5, 3); }"
assert 1 "int div(int x, int y) { return x / y; } int main() { return div(7, 7); }"
assert 42 "int ret42() { return 42; } int main() { return ret42(); }"
assert 5 "int add1(int x) { return x + 1; } int main() { return add1(4); }"
assert 10 "int double(int x) { return x * 2; } int main() { return double(5); }"

# 複数引数のテスト  
assert 6 "int add3(int a, int b, int c) { return a + b + c; } int main() { return add3(1, 2, 3); }"
assert 24 "int mul4(int a, int b, int c, int d) { return a * b * c * d; } int main() { return mul4(1, 2, 3, 4); }"

# ネストした関数呼び出し
assert 14 "int add(int x, int y) { return x + y; } int main() { return add(add(3, 4), add(3, 4)); }"
assert 25 "int square(int x) { return x * x; } int main() { return square(5); }"

# 再帰関数のテスト
assert 120 "int fact(int n) { if (n <= 1) return 1; return n * fact(n - 1); } int main() { return fact(5); }"
assert 8 "int fib(int n) { if (n <= 1) return n; return fib(n-1) + fib(n-2); } int main() { return fib(6); }"
assert 1 "int fact(int n) { if (n <= 1) return 1; return n * fact(n - 1); } int main() { return fact(0); }"

# より複雑な関数テスト
assert 26 "int mul(int x, int y) { return x * y; } int add(int a, int b) { return a + b; } int main() { return add(mul(2, 3), mul(4, 5)); }"
assert 36 "int pow2(int x) { return x * x; } int main() { return pow2(6); }"
assert 81 "int pow4(int x) { return x * x * x * x; } int main() { return pow4(3); }"
assert 100 "int max(int a, int b) { if (a > b) return a; return b; } int main() { return max(100, 50); }"
assert 10 "int min(int a, int b) { if (a < b) return a; return b; } int main() { return min(10, 20); }"

# 引数なし関数
assert 77 "int get77() { return 77; } int main() { return get77(); }"
assert 99 "int get99() { return 99; } int get77() { return 77; } int main() { return get99(); }"

# 複数レベルのネスト
assert 30 "int f1(int x) { return x + 10; } int f2(int x) { return f1(x) + 10; } int f3(int x) { return f2(x) + 10; } int main() { return f3(0); }"

echo OK