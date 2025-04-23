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

assert 0 0
assert 42 42
assert 6 "4+2"
assert 21 "5+20-4"
assert 41 "12 + 34 - 5"
assert 17 "3*4+5"
assert 3 "(5+7)/4"
assert 15 "5*(9-6)"
assert 3 "-3+6"

echo OK