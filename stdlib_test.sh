#!/bin/bash

# 標準ライブラリ機能テストスクリプト
# test.shのGCC比較方式を使用

# test.shからtest_gcc関数を定義
test_gcc(){
    program="$1"
    
    # 作ったコンパイラでテスト
    ./mipsc "$program" > tmp_our.s 2>/dev/null
    if ! mips-linux-gnu-gcc -mno-abicalls -fno-pic tmp_our.s -o tmp_our -nostdlib -static 2>/dev/null; then
        echo "❌ COMPILE FAILED: $program"
        return 1
    fi
    qemu-mips tmp_our 2>/dev/null
    our_exit=$?
    
    # 標準GCCでテスト
    echo "$program" > tmp_gcc.c
    if ! mips-linux-gnu-gcc tmp_gcc.c -o tmp_gcc -static 2>/dev/null; then
        echo "❌ GCC COMPILE FAILED: $program"
        return 1
    fi
    qemu-mips tmp_gcc 2>/dev/null
    gcc_exit=$?
    
    # 比較
    if [ "$our_exit" = "$gcc_exit" ]; then
        echo "✅ $program => $our_exit"
    else
        echo "❌ $program => Our: $our_exit, GCC: $gcc_exit"
        return 1
    fi
}

# printf専用テスト関数（出力内容も検証）
test_printf_output(){
    program="$1"
    description="$2"
    
    echo "=== Testing: $description ==="
    
    # 我々のコンパイラでテスト
    echo "Our compiler:"
    ./mipsc "$program" > tmp_our.s 2>/dev/null
    if mips-linux-gnu-gcc -mno-abicalls -fno-pic tmp_our.s -o tmp_our -nostdlib -static 2>/dev/null; then
        our_output=$(qemu-mips tmp_our 2>&1)
        our_exit=$?
        echo "Output: '$our_output'"
        echo "Exit code: $our_exit"
    else
        echo "❌ Compilation failed"
        return 1
    fi
    
    echo ""
    
    # 標準GCCでテスト（比較用）
    echo "Standard GCC:"
    echo "$program" > tmp_gcc.c
    if mips-linux-gnu-gcc tmp_gcc.c -o tmp_gcc -static 2>/dev/null; then
        gcc_output=$(qemu-mips tmp_gcc 2>&1)
        gcc_exit=$?
        echo "Output: '$gcc_output'"
        echo "Exit code: $gcc_exit"
    else
        echo "❌ Compilation failed"
        return 1
    fi
    
    echo ""
    
    # 比較
    if [ "$our_output" = "$gcc_output" ] && [ "$our_exit" = "$gcc_exit" ]; then
        echo "✅ Output and exit codes match"
        echo "✅ $description: PASSED"
    else
        echo "❌ Output or exit codes differ"
        echo "❌ $description: FAILED"
        return 1
    fi
    
    echo "==============================================="
    echo ""
}

echo "########################################"
echo "#     標準ライブラリ機能テストスイート     #"
echo "########################################"
echo ""

echo "=== PART 1: printf基本機能テスト ==="
echo ""

# 基本的な文字列出力
test_printf_output 'int main() { printf("Hello"); return 0; }' "Basic string output"
test_printf_output 'int main() { printf("Hello, World"); return 0; }' "String with comma and space"

# エスケープシーケンス
test_printf_output 'int main() { printf("Line1\nLine2"); return 0; }' "Newline escape sequence"
test_printf_output 'int main() { printf("Tab\tSeparated"); return 0; }' "Tab escape sequence"

# 複数のprintf呼び出し
test_printf_output 'int main() { printf("Hello"); printf(" World"); return 0; }' "Multiple printf calls"
test_printf_output 'int main() { printf("A"); printf("B"); printf("C"); return 0; }' "Three printf calls"

echo ""
echo "=== PART 2: printf整数フォーマットテスト ==="
echo ""

# 基本的な%d
test_printf_output 'int main() { printf("%d", 42); return 0; }' "Basic integer formatting"
test_printf_output 'int main() { printf("%d", 0); return 0; }' "Zero formatting"
test_printf_output 'int main() { printf("%d", -5); return 0; }' "Negative integer formatting"

# 文字列と組み合わせ
test_printf_output 'int main() { printf("Number: %d", 123); return 0; }' "String with integer"
test_printf_output 'int main() { printf("Result is %d points", 87); return 0; }' "Integer in sentence"

# 変数を使用
test_printf_output 'int main() { int x = 99; printf("Value: %d", x); return 0; }' "Variable formatting"

echo ""
echo "=== PART 3: printf式とフォーマットテスト ==="
echo ""

# 式の結果をフォーマット
test_printf_output 'int main() { printf("%d", 3 + 4); return 0; }' "Expression formatting"
test_printf_output 'int main() { printf("%d", 10 * 5); return 0; }' "Multiplication formatting"
test_printf_output 'int main() { int x = 7; printf("%d", x * 2); return 0; }' "Variable expression formatting"

# 複数の%dテスト（制限確認）
echo "Testing multiple %d formatting (current limit: 3 args after format string)..."
./mipsc 'int main() { printf("%d %d", 1, 2); return 0; }' > tmp.s 2>/dev/null
if mips-linux-gnu-gcc -mno-abicalls -fno-pic tmp.s -o tmp -nostdlib -static 2>/dev/null; then
    output=$(qemu-mips tmp 2>&1)
    echo "✅ Two integers: '$output'"
else
    echo "❌ Two integers failed to compile"
fi

echo ""
echo "=== PART 4: printf制限と非対応機能テスト ==="
echo ""

echo "Testing printf limitations and unsupported features..."

# 非対応フォーマット指定子
echo "Unsupported format specifiers (should fail or produce unexpected results):"

echo -n "Testing %s (string): "
./mipsc 'int main() { printf("%s", "test"); return 0; }' > tmp.s 2>/dev/null
if mips-linux-gnu-gcc -mno-abicalls -fno-pic tmp.s -o tmp -nostdlib -static 2>/dev/null; then
    output=$(qemu-mips tmp 2>&1)
    echo "Output: '$output' (likely incorrect)"
else
    echo "Compilation failed (expected)"
fi

echo -n "Testing %c (character): "
./mipsc 'int main() { printf("%c", 65); return 0; }' > tmp.s 2>/dev/null
if mips-linux-gnu-gcc -mno-abicalls -fno-pic tmp.s -o tmp -nostdlib -static 2>/dev/null; then
    output=$(qemu-mips tmp 2>&1)
    echo "Output: '$output' (likely incorrect)"
else
    echo "Compilation failed (expected)"
fi

echo ""
echo "=== PART 5: 他の標準ライブラリ関数テスト ==="
echo ""

echo "Testing other potential standard library functions..."

# scanf（期待：非対応）
echo -n "Testing scanf: "
./mipsc 'int main() { int x; scanf("%d", &x); return x; }' > tmp.s 2>/dev/null
if mips-linux-gnu-gcc -mno-abicalls -fno-pic tmp.s -o tmp -nostdlib -static 2>/dev/null; then
    echo "Compiled (unexpected - scanf should not be implemented)"
else
    echo "❌ Not implemented (expected)"
fi

# strlen（期待：非対応）
echo -n "Testing strlen: "
./mipsc 'int main() { return strlen("hello"); }' > tmp.s 2>/dev/null
if mips-linux-gnu-gcc -mno-abicalls -fno-pic tmp.s -o tmp -nostdlib -static 2>/dev/null; then
    echo "Compiled (unexpected - strlen should not be implemented)"
else
    echo "❌ Not implemented (expected)"
fi

# malloc（期待：非対応）
echo -n "Testing malloc: "
./mipsc 'int main() { int* p = malloc(4); return 0; }' > tmp.s 2>/dev/null
if mips-linux-gnu-gcc -mno-abicalls -fno-pic tmp.s -o tmp -nostdlib -static 2>/dev/null; then
    echo "Compiled (unexpected - malloc should not be implemented)"
else
    echo "❌ Not implemented (expected)"
fi

echo ""
echo "=== PART 6: printf機能の境界テスト ==="
echo ""

# 引数制限テスト
echo "Testing printf argument limits..."

echo -n "4 arguments total (3 + format): "
./mipsc 'int main() { printf("%d %d %d", 1, 2, 3); return 0; }' > tmp.s 2>/dev/null
if mips-linux-gnu-gcc -mno-abicalls -fno-pic tmp.s -o tmp -nostdlib -static 2>/dev/null; then
    output=$(qemu-mips tmp 2>&1)
    echo "Output: '$output'"
else
    echo "Compilation failed"
fi

echo -n "5 arguments total (4 + format) - should exceed limit: "
./mipsc 'int main() { printf("%d %d %d %d", 1, 2, 3, 4); return 0; }' > tmp.s 2>/dev/null
if mips-linux-gnu-gcc -mno-abicalls -fno-pic tmp.s -o tmp -nostdlib -static 2>/dev/null; then
    output=$(qemu-mips tmp 2>&1)
    echo "Output: '$output' (likely missing last argument)"
else
    echo "Compilation failed"
fi

# 空文字列
test_printf_output 'int main() { printf(""); return 0; }' "Empty string"

# 長い文字列
test_printf_output 'int main() { printf("This is a relatively long string to test string handling"); return 0; }' "Long string"

echo ""
echo "=== PART 7: printf実用パターンテスト ==="
echo ""

# 実際のプログラムでよく使われるパターン
test_printf_output 'int main() { int i; for (i = 1; i <= 3; i++) printf("%d ", i); return 0; }' "Loop with printf"

# 条件分岐とprintf
test_printf_output 'int main() { int x = 5; if (x > 3) printf("Large"); else printf("Small"); return 0; }' "Conditional printf"

# 関数からの戻り値
test_printf_output 'int add(int a, int b) { return a + b; } int main() { printf("Sum: %d", add(3, 4)); return 0; }' "Function result formatting"

echo ""
echo "########################################"
echo "#        標準ライブラリテスト完了       #"
echo "########################################"

# クリーンアップ
rm -f tmp*.c tmp*.s tmp_our tmp_gcc tmp 2>/dev/null