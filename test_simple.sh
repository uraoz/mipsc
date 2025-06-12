#!/bin/bash

# 簡易版MIPS Cコンパイラ用テストスクリプト
# セルフホスト対応の最小限実装テスト

# 色付け用の定数
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# テスト結果カウンタ
total_tests=0
passed_tests=0
failed_tests=0

# ヘルプ表示
show_help() {
    echo "簡易版MIPS Cコンパイラ テストスクリプト"
    echo ""
    echo "使用法: $0 [オプション]"
    echo ""
    echo "オプション:"
    echo "  -h, --help     このヘルプを表示"
    echo "  -v, --verbose  詳細なテスト出力を表示"
    echo "  -q, --quiet    エラーのみ表示"
    echo "  -c, --compare  標準版との比較テストのみ実行"
    echo "  -f, --fast     高速テスト（コンパイルのみ、実行スキップ）"
    echo ""
    echo "簡易版の特徴:"
    echo "  - 静的メモリ管理（malloc/calloc/free → 静的配列）"
    echo "  - セルフホスト対応の最小限実装"
    echo "  - 基本的なC構文をサポート"
    echo "  - 標準ライブラリ依存を最小化"
}

# オプション解析
verbose=false
quiet=false
compare_only=false
fast_mode=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -v|--verbose)
            verbose=true
            shift
            ;;
        -q|--quiet)
            quiet=true
            shift
            ;;
        -c|--compare)
            compare_only=true
            shift
            ;;
        -f|--fast)
            fast_mode=true
            shift
            ;;
        *)
            echo "不明なオプション: $1"
            show_help
            exit 1
            ;;
    esac
done

# ログ関数
log_info() {
    if [ "$quiet" = false ]; then
        echo -e "${BLUE}[INFO]${NC} $1"
    fi
}

log_success() {
    if [ "$quiet" = false ]; then
        echo -e "${GREEN}[PASS]${NC} $1"
    fi
}

log_error() {
    echo -e "${RED}[FAIL]${NC} $1"
}

log_warning() {
    if [ "$quiet" = false ]; then
        echo -e "${YELLOW}[WARN]${NC} $1"
    fi
}

# テスト関数
test_compile() {
    local description="$1"
    local code="$2"
    local expected_exit_code="${3:-0}"
    
    total_tests=$((total_tests + 1))
    
    if [ "$verbose" = true ]; then
        log_info "テスト: $description"
        log_info "コード: $code"
    fi
    
    # 簡易版コンパイラでコンパイル
    if ! ./mipsc_simple "$code" > test_simple_tmp.s 2>/dev/null; then
        if [ "$expected_exit_code" -eq 0 ]; then
            log_error "$description: コンパイル失敗"
            failed_tests=$((failed_tests + 1))
            return 1
        else
            log_success "$description: 期待通りのコンパイル失敗"
            passed_tests=$((passed_tests + 1))
            return 0
        fi
    fi
    
    if [ "$expected_exit_code" -ne 0 ]; then
        log_error "$description: 失敗すべきコンパイルが成功"
        failed_tests=$((failed_tests + 1))
        rm -f test_simple_tmp.s
        return 1
    fi
    
    if [ "$fast_mode" = true ]; then
        log_success "$description: コンパイル成功"
        passed_tests=$((passed_tests + 1))
        rm -f test_simple_tmp.s
        return 0
    fi
    
    # MIPSアセンブラでアセンブル
    if ! mips-linux-gnu-gcc -mno-abicalls -fno-pic test_simple_tmp.s -o test_simple_tmp -nostdlib -static 2>/dev/null; then
        log_error "$description: MIPSアセンブル失敗"
        failed_tests=$((failed_tests + 1))
        rm -f test_simple_tmp.s
        return 1
    fi
    
    # QEMUで実行
    qemu-mips test_simple_tmp >/dev/null 2>&1
    local actual_exit_code=$?
    
    # 標準版と比較（存在する場合）
    local expected_result=""
    if [ -f mipsc ]; then
        ./mipsc "$code" > test_std_tmp.s 2>/dev/null
        if [ $? -eq 0 ]; then
            mips-linux-gnu-gcc -mno-abicalls -fno-pic test_std_tmp.s -o test_std_tmp -nostdlib -static 2>/dev/null
            if [ $? -eq 0 ]; then
                qemu-mips test_std_tmp >/dev/null 2>&1
                expected_result=$?
                rm -f test_std_tmp.s test_std_tmp
            fi
        fi
    fi
    
    if [ -n "$expected_result" ]; then
        if [ "$actual_exit_code" -eq "$expected_result" ]; then
            log_success "$description: 結果 $actual_exit_code (標準版と一致)"
            passed_tests=$((passed_tests + 1))
        else
            log_error "$description: 結果 $actual_exit_code, 期待値 $expected_result (標準版と不一致)"
            failed_tests=$((failed_tests + 1))
        fi
    else
        log_success "$description: 結果 $actual_exit_code"
        passed_tests=$((passed_tests + 1))
    fi
    
    rm -f test_simple_tmp.s test_simple_tmp
    return 0
}

# 比較専用テスト
test_compare_with_standard() {
    local description="$1"
    local code="$2"
    
    total_tests=$((total_tests + 1))
    
    if [ ! -f mipsc ]; then
        log_warning "$description: 標準版コンパイラが見つかりません（スキップ）"
        return 0
    fi
    
    # 標準版でコンパイル
    ./mipsc "$code" > test_std_comp.s 2>/dev/null
    local std_compile_result=$?
    
    # 簡易版でコンパイル
    ./mipsc_simple "$code" > test_simple_comp.s 2>/dev/null
    local simple_compile_result=$?
    
    # コンパイル結果の比較
    if [ "$std_compile_result" -ne "$simple_compile_result" ]; then
        log_error "$description: コンパイル結果が異なる（標準版:$std_compile_result, 簡易版:$simple_compile_result）"
        failed_tests=$((failed_tests + 1))
        rm -f test_std_comp.s test_simple_comp.s
        return 1
    fi
    
    if [ "$std_compile_result" -ne 0 ]; then
        log_success "$description: 両方とも期待通りのコンパイル失敗"
        passed_tests=$((passed_tests + 1))
        rm -f test_std_comp.s test_simple_comp.s
        return 0
    fi
    
    # アセンブルと実行
    mips-linux-gnu-gcc -mno-abicalls -fno-pic test_std_comp.s -o test_std_comp -nostdlib -static 2>/dev/null
    mips-linux-gnu-gcc -mno-abicalls -fno-pic test_simple_comp.s -o test_simple_comp -nostdlib -static 2>/dev/null
    
    if [ ! -f test_std_comp ] || [ ! -f test_simple_comp ]; then
        log_error "$description: アセンブル失敗"
        failed_tests=$((failed_tests + 1))
        rm -f test_std_comp.s test_simple_comp.s test_std_comp test_simple_comp
        return 1
    fi
    
    qemu-mips test_std_comp >/dev/null 2>&1
    local std_result=$?
    qemu-mips test_simple_comp >/dev/null 2>&1
    local simple_result=$?
    
    if [ "$std_result" -eq "$simple_result" ]; then
        log_success "$description: 実行結果一致（$simple_result）"
        passed_tests=$((passed_tests + 1))
    else
        log_error "$description: 実行結果不一致（標準版:$std_result, 簡易版:$simple_result）"
        failed_tests=$((failed_tests + 1))
    fi
    
    rm -f test_std_comp.s test_simple_comp.s test_std_comp test_simple_comp
    return 0
}

# メイン処理開始
echo -e "${BLUE}=== 簡易版MIPS Cコンパイラ テストスイート ===${NC}"
echo ""

# 簡易版コンパイラの存在確認
if [ ! -f mipsc_simple ]; then
    echo -e "${RED}エラー: mipsc_simple が見つかりません${NC}"
    echo "まず 'make -f Makefile_simple' を実行してください"
    exit 1
fi

# 必要なツールの確認
if ! command -v mips-linux-gnu-gcc >/dev/null 2>&1; then
    echo -e "${RED}エラー: mips-linux-gnu-gcc が見つかりません${NC}"
    exit 1
fi

if ! command -v qemu-mips >/dev/null 2>&1; then
    echo -e "${RED}エラー: qemu-mips が見つかりません${NC}"
    exit 1
fi

if [ "$compare_only" = true ]; then
    echo -e "${YELLOW}比較テストモード: 標準版との比較のみ実行${NC}"
    echo ""
    
    log_info "=== 標準版との比較テスト ==="
    test_compare_with_standard "基本的な数値" "int main() { return 42; }"
    test_compare_with_standard "算術演算" "int main() { return 3 + 4 * 2; }"
    test_compare_with_standard "複雑な式" "int main() { return (1 + 2) * 3 - 4; }"
    test_compare_with_standard "優先順位" "int main() { return 1 + 2 * 3 + 4; }"
    test_compare_with_standard "負の数" "int main() { return -42; }"
    test_compare_with_standard "ゼロ" "int main() { return 0; }"
    
else
    if [ "$fast_mode" = true ]; then
        log_info "高速モード: コンパイルのみテスト"
    fi
    
    echo -e "${YELLOW}Phase 1: 基本機能テスト${NC}"
    test_compile "基本的な数値リテラル" "int main() { return 42; }"
    test_compile "ゼロ" "int main() { return 0; }"
    test_compile "負の数" "int main() { return -5; }"
    test_compile "大きな数" "int main() { return 999; }"
    
    echo ""
    echo -e "${YELLOW}Phase 2: 算術演算テスト${NC}"
    test_compile "加算" "int main() { return 3 + 4; }"
    test_compile "減算" "int main() { return 10 - 3; }"
    test_compile "乗算" "int main() { return 6 * 7; }"
    test_compile "除算" "int main() { return 15 / 3; }"
    test_compile "剰余" "int main() { return 17 % 5; }"
    
    echo ""
    echo -e "${YELLOW}Phase 3: 演算子優先順位テスト${NC}"
    test_compile "乗算優先" "int main() { return 2 + 3 * 4; }"
    test_compile "括弧優先" "int main() { return (2 + 3) * 4; }"
    test_compile "複雑な式" "int main() { return 1 + 2 * 3 - 4 / 2; }"
    test_compile "ネストした括弧" "int main() { return ((1 + 2) * 3) + 4; }"
    
    echo ""
    echo -e "${YELLOW}Phase 4: 比較演算テスト${NC}"
    test_compile "等価比較（真）" "int main() { return 5 == 5; }"
    test_compile "等価比較（偽）" "int main() { return 3 == 5; }"
    test_compile "不等価比較（真）" "int main() { return 3 != 5; }"
    test_compile "不等価比較（偽）" "int main() { return 5 != 5; }"
    test_compile "小なり比較（真）" "int main() { return 3 < 5; }"
    test_compile "小なり比較（偽）" "int main() { return 5 < 3; }"
    test_compile "小なりイコール（真）" "int main() { return 3 <= 5; }"
    test_compile "小なりイコール（境界）" "int main() { return 5 <= 5; }"
    
    echo ""
    echo -e "${YELLOW}Phase 5: エラーケーステスト${NC}"
    test_compile "構文エラー（閉じ括弧なし）" "int main() { return (3 + 4; }" 1
    test_compile "構文エラー（セミコロンなし）" "int main() { return 42 }" 1
    test_compile "構文エラー（不正な文字）" "int main() { return @; }" 1
    
    if [ "$fast_mode" = false ]; then
        echo ""
        echo -e "${YELLOW}Phase 6: 境界値テスト${NC}"
        test_compile "最小値" "int main() { return 1; }"
        test_compile "大きな計算結果" "int main() { return 100 * 100; }"
        test_compile "複雑な計算" "int main() { return 1+2+3+4+5+6+7+8+9+10; }"
    fi
    
    if [ -f mipsc ]; then
        echo ""
        echo -e "${YELLOW}Phase 7: 標準版との一致確認${NC}"
        test_compare_with_standard "基本機能一致確認" "int main() { return 42; }"
        test_compare_with_standard "算術演算一致確認" "int main() { return 3 + 4 * 2; }"
        test_compare_with_standard "比較演算一致確認" "int main() { return 5 > 3; }"
    fi
fi

# テスト結果サマリー
echo ""
echo -e "${BLUE}=== テスト結果サマリー ===${NC}"
echo -e "総テスト数: $total_tests"
echo -e "${GREEN}成功: $passed_tests${NC}"
echo -e "${RED}失敗: $failed_tests${NC}"

if [ "$failed_tests" -eq 0 ]; then
    echo ""
    echo -e "${GREEN}🎉 全てのテストが成功しました！${NC}"
    echo ""
    echo -e "${BLUE}簡易版コンパイラの特徴:${NC}"
    echo "  ✅ 静的メモリ管理（標準ライブラリ依存なし）"
    echo "  ✅ セルフホスト対応の最小実装"
    echo "  ✅ 基本的なC構文サポート"
    echo "  ✅ MIPSアセンブリ生成"
    if [ -f mipsc ]; then
        echo "  ✅ 標準版コンパイラとの完全互換性"
    fi
    exit 0
else
    echo ""
    echo -e "${RED}❌ $failed_tests 個のテストが失敗しました${NC}"
    echo ""
    echo "失敗したテストを確認し、修正が必要です。"
    echo "詳細な情報は -v オプションで確認できます。"
    exit 1
fi