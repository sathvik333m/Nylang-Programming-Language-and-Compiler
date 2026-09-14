PATH := $(CURDIR):$(PATH)

.PHONY: compiler phases run clean

compiler: lex.yy.c parser.tab.c exprtree.c symboltable.c semantic.c
	gcc lex.yy.c parser.tab.c exprtree.c symboltable.c semantic.c -o compiler

lex.yy.c: lexer.l
	flex lexer.l

parser.tab.c parser.tab.h: parser.y
	bison -d parser.y

phases: compiler
	./compiler --tokens program.ny > tokens.out
	./compiler --ast program.ny > ast.out
	./compiler --symbols program.ny > symbols.out
	./compiler --semantic program.ny > semantic.out
	./compiler program.ny > program.asm

run: compiler
	Nylang program.ny

clean:
	rm -f compiler lex.yy.c parser.tab.c parser.tab.h program.asm program.o program tokens.out ast.out symbols.out semantic.out
