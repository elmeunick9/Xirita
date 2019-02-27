CC=gcc

lexer: main.o lexer.o buffer.o ast.o parser.o sdt_run.o structs.o  utils.h sdt_run.h structs.h
	$(CC) -o xrun main.o buffer.o lexer.o parser.o sdt_run.o structs.o ast.o
	
clean:
	rm -f *.o *~ 