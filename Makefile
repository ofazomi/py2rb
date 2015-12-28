all:
	#lex test.yy
	#gcc -o test -ll lex.yy.c
	lex python.l
	bison --defines python.y
	#gcc -o pythontest -ll lex.yy.c
	#gcc -o pythontest -ly python.tab.c lex.yy.c
	gcc -o py2rb py2rb.c rubygrammar.c rubyprinter.c python.tab.c lex.yy.c
debug:
	lex python.l
	bison --debug --verbose --defines python.y
	gcc -c -g -DDEBUG py2rb.c rubygrammar.c rubyprinter.c
	gcc -c python.tab.c lex.yy.c
	gcc -o py2rb py2rb.o rubygrammar.o rubyprinter.o python.tab.o lex.yy.o
clean:
	-rm *.o
