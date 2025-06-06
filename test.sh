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
echo OK