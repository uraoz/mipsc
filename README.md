# MIPSC - C-to-MIPS Compiler

MIPSCは、Cのサブセットを解釈してMIPSアセンブリコードを生成するコンパイラです。

## コンパイル時の処理フロー

### 1. 字句解析（Lexical Analysis）- `parse.c:tokenize()`

入力されたCソースコードを意味のある単位（トークン）に分解します。

```
入力: "int x = 42;"
↓
トークン列: [TK_INT, TK_IDENT("x"), TK_RESERVED("="), TK_NUM(42), TK_RESERVED(";")]
```

**認識可能なトークン**:
- **キーワード**: `int`, `char`, `if`, `else`, `while`, `for`, `return`, `sizeof`
- **演算子**: `+`, `-`, `*`, `/`, `=`, `==`, `!=`, `<`, `<=`, `>`, `>=`, `++`, `--`, `+=`, `-=`, `*=`, `/=`, `&&`, `||`, `!`, `?`, `:`
- **区切り文字**: `(`, `)`, `{`, `}`, `[`, `]`, `;`, `,`
- **リテラル**: 整数、文字、文字列
- **識別子**: 変数名、関数名

### 2. 構文解析（Syntax Analysis）- `parse.c` 各種パーサー関数

トークン列から抽象構文木（AST）を構築します。再帰下降構文解析を使用。

#### 文法定義（拡張BNF記法）

```bnf
program    ::= (global_var | function)*
function   ::= type ident "(" (type ident ("," type ident)*)? ")" "{" stmt* "}"
global_var ::= type ident ("[" num "]")? ";"

stmt       ::= "return" expr ";"
             | "if" "(" expr ")" stmt ("else" stmt)?
             | "while" "(" expr ")" stmt  
             | "for" "(" expr? ";" expr? ";" expr? ")" stmt
             | "{" stmt* "}"
             | type ident ("[" num "]")? ("=" expr)? ";"
             | expr ";"

expr       ::= ternary
ternary    ::= assign ("?" expr ":" ternary)?
assign     ::= logical_or (("=" | "+=" | "-=" | "*=" | "/=") assign)?
logical_or ::= logical_and ("||" logical_and)*
logical_and::= equality ("&&" equality)*
equality   ::= relational (("==" | "!=") relational)*
relational ::= add (("<" | "<=" | ">" | ">=") add)*
add        ::= mul (("+" | "-") mul)*
mul        ::= unary (("*" | "/") unary)*
unary      ::= ("+" | "-" | "*" | "&" | "!" | "++" | "--")? unary
             | postfix
postfix    ::= primary ("[" expr "]" | "(" (expr ("," expr)*)? ")" | "++" | "--")*
primary    ::= "(" expr ")"
             | ident
             | num
             | string
             | char
             | "sizeof" "(" (type | unary) ")"

type       ::= ("int" | "char") "*"*
```

#### パーサーの階層構造

```
1. program()      # プログラム全体
2. stmt()         # 文の解析
3. expr()         # 式の解析（最上位）
4. ternary()      # 三項演算子 (? :)
5. assign()       # 代入演算子 (=, +=, -=, *=, /=)  
6. logical_or()   # 論理OR (||)
7. logical_and()  # 論理AND (&&)
8. equality()     # 等価比較 (==, !=)
9. relational()   # 大小比較 (<, <=, >, >=)
10. add()         # 加減算 (+, -)
11. mul()         # 乗除算 (*, /)
12. unary()       # 単項演算子 (+, -, *, &, !, ++, --)
13. postfix()     # 後置演算子 ([], (), ++, --)
14. primary()     # 基本要素 (識別子、数値、括弧式)
```

### 3. 意味解析（Semantic Analysis）- `parse.c` 型システム

ASTに型情報を付与し、型の整合性をチェックします。

**サポートする型**:
- `int`: 32ビット整数
- `char`: 8ビット文字
- `int*`, `char*`: ポインタ型
- `int[N]`, `char[N]`: 配列型

**型推論の例**:
```c
int* ptr;           // ptr の型: int*
int arr[10];        // arr の型: int[10] → int* (配列はポインタに自動変換)
ptr + 1;            // ポインタ演算: ptr + (1 * sizeof(int))
sizeof(arr);        // 配列全体のサイズ: 10 * 4 = 40
```

### 4. コード生成（Code Generation）- `codegen.c`

ASTを再帰的に辿り、MIPSアセンブリコードを生成します。

#### レジスタ使用規則

- `$t0`, `$t1`: 一時計算用
- `$sp`: スタックポインタ
- `$s8` (`$fp`): フレームポインタ
- `$a0-$a3`: 関数引数
- `$v0`: 関数戻り値
- `$ra`: リターンアドレス

#### スタックフレーム構造

```
高いアドレス
+------------------+
| 引数4以降        |
+------------------+
| リターンアドレス | <- $ra 保存
+------------------+
| 旧フレームポインタ| <- $s8 保存  
+------------------+ <- 新しい $s8
| ローカル変数1    |
| ローカル変数2    |
| ...              |
+------------------+
| 一時的な値       | <- $sp
+------------------+
低いアドレス
```

#### 主要な生成パターン

**1. 算術演算**:
```c
// C: a + b
gen(a);                    // a の値をスタックにプッシュ
gen(b);                    // b の値をスタックにプッシュ  
lw $t1, 0($sp)            // b をロード
addiu $sp, $sp, 4         // スタックポップ
lw $t0, 0($sp)            // a をロード
addiu $sp, $sp, 4         // スタックポップ
add $t0, $t0, $t1         // a + b を計算
addiu $sp, $sp, -4        // 結果をプッシュ
sw $t0, 0($sp)
```

**2. 条件分岐**:
```c
// C: if (cond) then_stmt else else_stmt
gen(cond);                 // 条件式を評価
lw $t0, 0($sp)            // 条件の結果
addiu $sp, $sp, 4
beqz $t0, .Lelse{label}   // 偽なら else へ
gen(then_stmt);           // then 部分
j .Lend{label}            // 終了へジャンプ
.Lelse{label}:
gen(else_stmt);           // else 部分  
.Lend{label}:
```

**3. 三項演算子**:
```c
// C: cond ? then_expr : else_expr
gen(cond);                 // 条件式を評価
lw $t0, 0($sp)
addiu $sp, $sp, 4  
beqz $t0, .Lelse{label}   // 偽なら else_expr へ
gen(then_expr);           // then_expr を評価
j .Lend{label}
.Lelse{label}:
gen(else_expr);           // else_expr を評価
.Lend{label}:
```

**4. 関数呼び出し**:
```c
// C: func(arg1, arg2, arg3)
gen(arg1); gen(arg2); gen(arg3);  // 引数を評価
lw $a2, 0($sp); addiu $sp, $sp, 4 // arg3 → $a2
lw $a1, 0($sp); addiu $sp, $sp, 4 // arg2 → $a1  
lw $a0, 0($sp); addiu $sp, $sp, 4 // arg1 → $a0
jal func                          // 関数呼び出し
addiu $sp, $sp, -4               // 戻り値をスタックへ
sw $v0, 0($sp)
```

### 5. 最適化

現在実装されている最適化:

**リファクタリングによる効率化**:
- 複合代入演算子の共通処理統合（80%のコード削減）
- インクリメント/デクリメント演算子の統合（78%のコード削減）
- 変数アクセス処理の統合（82%のコード削減）

**将来的な最適化候補**:
- 定数折りたたみ（Constant Folding）
- デッドコード除去（Dead Code Elimination）  
- レジスタ割り当て最適化
- 命令スケジューリング

## サポート機能

### 基本機能
- ✅ 算術演算（+, -, *, /）
- ✅ 比較演算（==, !=, <, <=, >, >=）
- ✅ 論理演算（&&, ||, !）
- ✅ 代入演算（=, +=, -=, *=, /=）
- ✅ インクリメント/デクリメント（++, --）
- ✅ 三項演算子（? :）

### 制御構造
- ✅ if-else文
- ✅ while文
- ✅ for文
- ✅ ブロック文（{}）

### データ型と変数
- ✅ int, char型
- ✅ ポインタ型
- ✅ 配列型
- ✅ ローカル変数
- ✅ グローバル変数
- ✅ sizeof演算子

### 関数
- ✅ 関数定義
- ✅ 関数呼び出し
- ✅ 引数渡し
- ✅ 戻り値

### リテラル
- ✅ 整数リテラル
- ✅ 文字リテラル（エスケープシーケンス対応）
- ✅ 文字列リテラル

## ビルドと実行

```bash
# コンパイラをビルド
make

# テスト実行
make test

# 使用例
./mipsc "int main() { return 42; }" > output.s
mips-linux-gnu-gcc output.s -o program -nostdlib -static
qemu-mips program
echo $?  # 42 が出力される
```

## 実装統計

- **総行数**: 約2500行
- **ファイル構成**: 
  - `main.c`: メイン処理（50行）
  - `parse.c`: 字句解析・構文解析（1000行）
  - `codegen.c`: コード生成（600行）
  - `mipsc.h`: ヘッダファイル（300行）
  - `test.sh`: テストスイート（500行）
- **テストケース**: 100+個
- **サポート機能**: C言語の主要機能の80%以上
