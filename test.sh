#!/bin/bash

# 基本的なテスト関数（従来のassert）
assert(){
    expected="$1"
    input="$2"

    ./mipsc "$input" > tmp.s 2>/dev/null
    if ! mips-linux-gnu-gcc -mno-abicalls -fno-pic tmp.s -o tmp -nostdlib -static 2>/dev/null; then
        echo "❌ COMPILE FAILED: $input"
        return 1
    fi
    
    qemu-mips tmp 2>/dev/null
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "✅ $input => $actual"
    else
        echo "❌ $input => expected $expected, got $actual"
        return 1
    fi
}

# GCCとの比較テスト関数
compare_with_gcc(){
    program="$1"
    test_name="$2"
    
    echo "=== Testing: $test_name ==="
    
    # 我々のコンパイラでテスト
    echo "Our compiler:"
    ./mipsc "$program" > tmp_our.s 2>/dev/null
    if mips-linux-gnu-gcc -mno-abicalls -fno-pic tmp_our.s -o tmp_our -nostdlib -static 2>/dev/null; then
        echo -n "Output: "
        qemu-mips tmp_our 2>&1
        our_exit=$?
        echo "Exit code: $our_exit"
    else
        echo "❌ Compilation failed"
        our_exit=-1
    fi
    
    echo ""
    
    # 通常のCコンパイラでテスト
    echo "Standard GCC:"
    echo "$program" > tmp_gcc.c
    if mips-linux-gnu-gcc tmp_gcc.c -o tmp_gcc -static 2>/dev/null; then
        echo -n "Output: "
        qemu-mips tmp_gcc 2>&1
        gcc_exit=$?
        echo "Exit code: $gcc_exit"
    else
        echo "❌ Compilation failed"
        gcc_exit=-1
    fi
    
    echo ""
    
    # 比較結果
    if [ "$our_exit" = "$gcc_exit" ]; then
        echo "✅ Exit codes match: $our_exit"
        echo "✅ $test_name: PASSED"
    else
        echo "❌ Exit codes differ: Our=$our_exit, GCC=$gcc_exit"
        echo "❌ $test_name: FAILED"
    fi
    
    echo "==============================================="
    echo ""
}

# printf専用テスト関数
test_printf(){
    program="$1"
    test_name="$2"
    
    echo "=== Testing: $test_name ==="
    
    # 我々のコンパイラでprintf機能をテスト
    echo "Our compiler (printf functionality):"
    ./mipsc "$program" > tmp_printf.s 2>/dev/null
    if mips-linux-gnu-gcc -mno-abicalls -fno-pic tmp_printf.s -o tmp_printf -nostdlib -static 2>/dev/null; then
        echo -n "Output: "
        output=$(qemu-mips tmp_printf 2>&1)
        exit_code=$?
        echo "'$output'"
        echo "Exit code: $exit_code"
        
        if [[ "$output" == *"Hello"* ]] || [[ "$output" == *"Test"* ]] || [[ "$output" == *"World"* ]]; then
            echo "✅ printf output detected"
            echo "✅ $test_name: PASSED"
        else
            echo "❌ No expected printf output"
            echo "❌ $test_name: FAILED"
        fi
    else
        echo "❌ Compilation failed"
        echo "❌ $test_name: FAILED"
    fi
    
    echo "==============================================="
    echo ""
}

echo "########################################"
echo "#          MIPS Compiler Test Suite    #"
echo "########################################"
echo ""

echo "=== PART 1: 基本機能テスト ==="
echo ""

# 基本的なreturn文
assert 0 'int main() { return 0; }'
assert 42 'int main() { return 42; }'
assert 6 'int main() { return 4+2; }'

echo ""
echo "=== PART 2: ローカル変数テスト ==="
echo ""

# 基本的な変数宣言と代入
assert 1 'int main() { int a; a = 1; return a; }'
assert 42 'int main() { int x; x = 42; return x; }'
assert 3 'int main() { int a; int b; a = 1; b = 2; return a + b; }'

echo ""
echo "=== PART 3: 関数機能テスト ==="
echo ""

# 基本的な関数呼び出し
assert 7 'int plus(int x, int y) { return x + y; } int main() { return plus(3, 4); }'
assert 15 'int mul(int x, int y) { return x * y; } int main() { return mul(3, 5); }'

echo ""
echo "=== PART 4: 配列・ポインタテスト ==="
echo ""

# 基本的な配列操作
assert 1 'int main() { int a[2]; *a = 1; return *a; }'
assert 42 'int main() { int arr[3]; arr[0] = 42; return arr[0]; }'
assert 20 'int main() { int arr[5]; return sizeof(arr); }'

echo ""
echo "=== PART 5: printf機能テスト ==="
echo ""

# printf機能専用テスト
test_printf 'int main() { printf("Hello, World!"); return 0; }' "Basic printf"
test_printf 'int main() { printf("Test"); printf(" Works"); return 42; }' "Multiple printf calls"

echo ""
echo "=== PART 6: GCCとの比較テスト ==="
echo ""

# 基本プログラムの比較
compare_with_gcc 'int main() { return 42; }' "Simple return"
compare_with_gcc 'int main() { int x; int y; x = 10; y = 5; return x + y; }' "Variable operations"
compare_with_gcc 'int add(int a, int b) { return a + b; } int main() { return add(3, 4); }' "Function calls"
compare_with_gcc 'int main() { int arr[3]; arr[0] = 1; arr[1] = 2; arr[2] = 3; return arr[0] + arr[1] + arr[2]; }' "Array operations"
compare_with_gcc 'int factorial(int n) { if (n <= 1) return 1; return n * factorial(n - 1); } int main() { return factorial(5); }' "Recursive function"

echo ""
echo "=== PART 7: 高度な機能テスト ==="
echo ""

# 複雑なテスト
assert 4 'int main() { return sizeof(int); }'
assert 1 'int main() { return sizeof(char); }'
assert 15 'int g; int main() { g = 5; return g + 10; }'
assert 99 'int g; int main() { int g; g = 99; return g; }'

echo ""
echo "=== PART 8: 文字リテラルテスト ==="
echo ""

# 基本的な文字リテラル
assert 65 'int main() { return '\''A'\''; }'
assert 48 'int main() { return '\''0'\''; }'
assert 122 'int main() { return '\''z'\''; }'

# エスケープシーケンス
assert 10 'int main() { return '\''\n'\''; }'
assert 9 'int main() { return '\''\t'\''; }'
assert 13 'int main() { return '\''\r'\''; }'
assert 92 'int main() { return '\''\\'\''; }'
echo "Testing single quote character literal..."
./mipsc "int main() { return '\\''; }" > tmp.s
mips-linux-gnu-gcc -mno-abicalls -fno-pic tmp.s -o tmp -nostdlib -static 2>/dev/null
result=$(qemu-mips tmp 2>/dev/null; echo $?)
if [ "$result" = "39" ]; then
    echo "✅ int main() { return '\\''; } => 39"
else
    echo "❌ int main() { return '\\''; } => expected 39, got $result"
fi
assert 0 'int main() { return '\''\0'\''; }'

# 文字リテラルの演算
assert 3 'int main() { return '\''C'\'' - '\''@'\''; }'
assert 10 'int main() { return '\''9'\'' - '\''/'\''; }'

echo ""
echo "=== PART 9: 文字列エスケープシーケンステスト ==="
echo ""

echo "=== Testing string escape sequences ==="

# 基本的な文字列エスケープ
echo "Testing newline in string..."
./mipsc 'int main() { printf("Line1\nLine2\n"); return 0; }' > tmp.s
mips-linux-gnu-gcc -mno-abicalls -fno-pic tmp.s -o tmp -nostdlib -static
output=$(qemu-mips tmp 2>&1)
if echo "$output" | grep -q "Line1" && echo "$output" | grep -q "Line2"; then
    echo "✅ String newline escape sequences work"
else
    echo "❌ String newline escape sequences failed"
fi

# タブ文字のテスト
echo "Testing tab in string..."
./mipsc 'int main() { printf("A\tB\tC"); return 0; }' > tmp.s
mips-linux-gnu-gcc -mno-abicalls -fno-pic tmp.s -o tmp -nostdlib -static
output=$(qemu-mips tmp 2>&1)
if echo "$output" | grep -q "A.*B.*C"; then
    echo "✅ String tab escape sequences work"
else
    echo "❌ String tab escape sequences failed"
fi

# 引用符のテスト
echo "Testing quote escape in string..."
./mipsc 'int main() { printf("Say \"Hello\""); return 0; }' > tmp.s
mips-linux-gnu-gcc -mno-abicalls -fno-pic tmp.s -o tmp -nostdlib -static
output=$(qemu-mips tmp 2>&1)
if echo "$output" | grep -q 'Say "Hello"'; then
    echo "✅ String quote escape sequences work"
else
    echo "❌ String quote escape sequences failed"
fi

echo ""
echo "=== PART 10: if-else文テスト ==="
echo ""

# 基本的なif-else文
assert 42 'int main() { if (1) return 42; else return 24; }'
assert 24 'int main() { if (0) return 42; else return 24; }'

# 変数を使ったif-else文
assert 5 'int main() { int x; x = 1; if (x) return 5; else return 10; }'
assert 10 'int main() { int x; x = 0; if (x) return 5; else return 10; }'

# 比較演算子を使ったif-else文
assert 1 'int main() { int x; x = 5; if (x > 3) return 1; else return 0; }'
assert 0 'int main() { int x; x = 2; if (x > 3) return 1; else return 0; }'

# ネストしたif-else文
assert 11 'int main() { int x; int y; x = 1; y = 1; if (x) { if (y) return 11; else return 12; } else return 20; }'
assert 12 'int main() { int x; int y; x = 1; y = 0; if (x) { if (y) return 11; else return 12; } else return 20; }'
assert 20 'int main() { int x; int y; x = 0; y = 1; if (x) { if (y) return 11; else return 12; } else return 20; }'

# 従来のif文（else無し）が引き続き動作することを確認
assert 77 'int main() { if (1) return 77; return 88; }'
assert 88 'int main() { if (0) return 77; return 88; }'

echo ""
echo "=== PART 11: 複合代入演算子テスト ==="
echo ""

# 基本的な複合代入演算子
assert 8 'int main() { int x; x = 5; x += 3; return x; }'
assert 7 'int main() { int x; x = 10; x -= 3; return x; }'
assert 12 'int main() { int x; x = 4; x *= 3; return x; }'
assert 5 'int main() { int x; x = 15; x /= 3; return x; }'

# 連続した複合代入演算子
assert 14 'int main() { int x; x = 2; x += 3; x *= 2; x += 4; return x; }'
assert 1 'int main() { int x; x = 10; x -= 5; x /= 2; x -= 1; return x; }'

# 変数同士の複合代入
assert 15 'int main() { int x; int y; x = 10; y = 5; x += y; return x; }'
assert 2 'int main() { int x; int y; x = 8; y = 4; x /= y; return x; }'

# 複合代入演算子の結果を使った式
assert 25 'int main() { int x; int y; x = 3; y = (x += 2) * 5; return y; }'

echo ""
echo "=== PART 12: ファイル入力テスト ==="
echo ""

echo "=== Testing file input functionality ==="

# sample.cファイルのテスト
echo "Testing sample.c compilation..."
./mipsc sample.c > tmp.s
mips-linux-gnu-gcc -mno-abicalls -fno-pic tmp.s -o tmp -nostdlib -static
result=$(qemu-mips tmp; echo $?)
if [ "$result" = "120" ]; then
    echo "✅ sample.c (factorial): $result"
else
    echo "❌ sample.c (factorial): expected 120, got $result"
fi

# test_input.cファイルのテスト  
echo "Testing test_input.c compilation..."
./mipsc test_input.c > tmp.s
mips-linux-gnu-gcc -mno-abicalls -fno-pic tmp.s -o tmp -nostdlib -static
result=$(qemu-mips tmp; echo $?)
if [ "$result" = "42" ]; then
    echo "✅ test_input.c: $result"
else
    echo "❌ test_input.c: expected 42, got $result"
fi

# 存在しないファイルのエラーテスト
echo "Testing error handling for non-existent file..."
if ./mipsc nonexistent.c 2>&1 | grep -q "cannot open"; then
    echo "✅ Error handling for missing files works"
else
    echo "❌ Error handling for missing files failed"
fi

# 文字列入力がまだ動くかテスト
echo "Testing direct string input still works..."
./mipsc "int main() { return 77; }" > tmp.s
mips-linux-gnu-gcc -mno-abicalls -fno-pic tmp.s -o tmp -nostdlib -static
result=$(qemu-mips tmp; echo $?)
if [ "$result" = "77" ]; then
    echo "✅ Direct string input: $result"
else
    echo "❌ Direct string input: expected 77, got $result"
fi

echo ""
echo "########################################"
echo "#          テスト完了                    #"
echo "########################################"

# クリーンアップ
rm -f tmp*.c tmp*.s tmp_our tmp_gcc tmp_printf tmp 2>/dev/null

