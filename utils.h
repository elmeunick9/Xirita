#ifndef XIRITA_UTILS_H
#define XIRITA_UTILS_H

#ifdef _WIN32
#include <io.h>
#define STDIN_FILENO _fileno(stdin)
#define STDOUT_FILENO _fileno(stdout)
#define read _read
#define write _write
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#else
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0

/* This library contains some utility functions for C */

typedef unsigned char byte;
typedef unsigned char bool;

static void swap(char** a, char** b) {
	char* t = *a;
	*a = *b;
	*b = t;
}

static void error(char* err) {
	puts("\n");
	puts(err);
	exit(1);
}

//Buffer
char* buff_init();
char* buff_next();
char* buff_back();
char* buff_rewind(unsigned int);
void buff_print(char*, char*);
char* buff_copy(char*, char*);
void buff_print_all();
unsigned int buff_distance(char*, char*);
size_t buff_get_count();

//Lexer
enum TokenID {L_STR, L_NUM, L_BOOL, SPACE, SEP, NL, OP_BLOCK, OP_INLINE, VNAME, FNAME, OFB, CFB, OVB, CVB, OSB, CSB, OP_MATH}; 
struct Token {
	enum TokenID id;
	char* lexeme_begin;
	char* lexeme_end;
	unsigned int line;
	unsigned int column;
} token;

void lex_init();
void lex_next();
void lex_print();
int  lex_sep(struct Token);

#endif