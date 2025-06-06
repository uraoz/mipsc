#!/bin/bash
assert(){
	expected="$1"
	input="$2"

	./mipsc "$input" >tmp.s
	mips-linux-gnu-gcc tmp.s -o tmp -nostdlib -static
	qemu-mips tmp
	actual="$?"

	if [ "$actual" = "$expected" ]; then
		echo "$input => $actual"
	else
		echo "$input => $expected expected, but got $actual"
		exit 1
	fi
}

assert 0 "0;"
assert 42 "42;"
assert 6 "4+2;"
assert 21 "5+20-4;"
assert 41 "12 + 34 - 5;"
assert 17 "3*4+5;"
assert 3 "(5+7)/4;"
assert 15 "5*(9-6);"
assert 3 "-3+6;"
assert 0 "0==1;"
assert 1 "3==3;"
assert 1 "0!=1;"
assert 1 "0<1;"
assert 0 "1<0;"
assert 1 "a=1;a;"
assert 2 "a=1;b=2;b;"
assert 3 "a=1;b=2;a+b;"
assert 6 "a=1;b=2;c=3;a+b+c;"
assert 5 "a=3;b=2;a+b;"
assert 1 "a=2;b=1;a-b;"
assert 10 "foo=10;foo;"
assert 15 "bar=5;baz=10;bar+baz;"
assert 42 "aaa=42;aaa;"
assert 100 "hello=50;world=50;hello+world;"
assert 1 "x1=1;y2=2;x1;"
assert 42 "return 42;"
assert 15 "a=5; b=10; return a+b;"
assert 7 "x=3; y=4; return x+y;"
assert 10 "foo=5; bar=2; return foo*bar;"
assert 100 "return 10*10;"
assert 5 "if (1) return 5;"
assert 10 "a=10; if (1) return a;"
assert 42 "if (0) return 5; return 42;"
assert 3 "a=3; if (a>0) return a; return 0;"
assert 0 "a=-1; if (a>0) return a; return 0;"
assert 5 "{return 5;}"
assert 10 "{a=10; return a;}"
assert 15 "{a=5; b=10; return a+b;}"
assert 3 "if (1) {a=3; return a;}"
assert 0 "if (0) {return 5;} return 0;"
assert 42 "{a=10; b=20; c=12; return a+b+c;}"
assert 10 "i=0; while (i<10) i=i+1; return i;"
assert 3 "a=0; i=0; while (i<3) {a=a+1; i=i+1;} return a;"
assert 55 "sum=0; i=1; while (i<=10) {sum=sum+i; i=i+1;} return sum;"
assert 0 "while (0) return 5; return 0;"
assert 0 "i=5; while (i>0) i=i-1; return i;"
assert 10 "for (i=0; i<10; i=i+1) ; return i;"
assert 3 "a=0; for (i=0; i<3; i=i+1) a=a+1; return a;"
assert 55 "sum=0; for (i=1; i<=10; i=i+1) sum=sum+i; return sum;"
assert 15 "total=0; for (i=1; i<=5; i=i+1) {total=total+i;} return total;"
assert 0 "for (i=10; i>0; i=i-1) ; return i;"
echo OK