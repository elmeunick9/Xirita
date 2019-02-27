#include "utils.h"

#define TAB_SIZE 4

char* forward;
char* lexeme_begin;
char* lexeme_name;
unsigned int line;
static size_t count;
bool sep_flag; //There may be only 1 SEP each line.
bool eof_flag; //File always ends with NL.

void lex_error(char* err) {
	printf(err); fflush(stdout);
	buff_print(lexeme_begin, forward);
	printf("\n");
	
	exit(1);
}

static void token_varname() {
	int state = 0;
	while(1) {
		switch(state) {
			case 0:
				if (!(*forward >= 'a' && *forward <= 'z')) return;
				state = 1; break;
			case 1:
				if (*forward >= 'a' && *forward <= 'z') break;
				if (*forward >= '0' && *forward <= '9') break;
				return;
		}
		forward = buff_next();
	}
}

static void token_funname() {
	int state = 0;
	while(1) {
		switch(state) {
			case 0:
				if (!(*forward >= 'A' && *forward <= 'Z')) return;
				state = 1; break;
			case 1:
				if (*forward >= 'A' && *forward <= 'Z') break;
				if (*forward >= '0' && *forward <= '9') break;
				return;
		}
		forward = buff_next();
	}
}

static void token_litbool() {
	bool word = TRUE;
	switch (*forward) {
		case 'T':
			forward = buff_next(); if (*forward != 'r') break; 
			forward = buff_next(); if (*forward != 'u') break; 
			forward = buff_next(); if (*forward != 'e') break;
			forward = buff_next();
			return;
		case 'F':
			forward = buff_next(); if (*forward != 'a') break; 
			forward = buff_next(); if (*forward != 'l') break; 
			forward = buff_next(); if (*forward != 's') break;
			forward = buff_next(); if (*forward != 'e') break;
			forward = buff_next();
			return;
	}
	forward = buff_rewind(buff_distance(lexeme_begin, forward));
}

static void token_litnum() {
	int state = 0;
	while(1) {
		if (!(*forward >= '0' && *forward <= '9')) return;
		forward = buff_next();
	}
}

static void token_litstr() {
	int state = 0;
	if (*forward != '"') return;
	forward = buff_next();
	
	while(*forward != '"') {
		forward = buff_next();
	}
	forward = buff_next();
	return;
}


static void token_moperator() {
	switch(*forward) {
		case '+': forward = buff_next(); break;
		case '-': forward = buff_next(); break;
	}
}

static void token_newline() {
	//Spacial cases for blank lines, that is, lines that only contain tabs or spaces.
	while (*forward == ' ' || *forward == '\t') {
		forward = buff_next();
	}
	
	if (*forward == '\n') { forward = buff_next(); return; }
	if (*forward == '\r') {
		forward = buff_next();
		if (*forward == '\n') { forward = buff_next(); return; }
	}
	if (*forward == '\0') { forward = buff_next(); return; }
	
	forward = buff_rewind(buff_distance(lexeme_begin, forward));
}

static void token_tab() {
	while(1) {
			if (*forward != '\t') break;
			forward = buff_next();
	}
}

static void token_space() {
	while(1) {
			if (*forward != ' ') break;
			forward = buff_next();
	}
}

static bool try_token(enum TokenID id) {
	if (buff_distance(lexeme_begin, forward) > 0) { token.id = id; return TRUE; }
	return FALSE;
}

static void match() {
	switch(*forward) {
		case '[': forward = buff_next(); token.id = OFB; sep_flag = TRUE; return;
		case ']': forward = buff_next(); token.id = CFB; return;
		case '(': forward = buff_next(); token.id = OVB; return;
		case ')': forward = buff_next(); token.id = CVB; return;
		case '}': forward = buff_next(); token.id = OSB; return;
		case '{': forward = buff_next(); token.id = CSB; return;
		case ':': forward = buff_next(); token.id = OP_BLOCK; return;
		case ',': forward = buff_next(); token.id = OP_INLINE; return;
	}
	
	token_newline(); if (try_token(NL)) { line++; count=buff_get_count(); sep_flag = FALSE; return; }
	token_space(); if (try_token(SPACE)) { sep_flag = TRUE; return; }
	token_tab(); if (try_token(SEP)) {
		if (sep_flag) token.id = SPACE;
		else sep_flag = TRUE;
		return; 
	}
	token_litbool(); if (try_token(L_BOOL)) return;
	token_litnum(); if (try_token(L_NUM)) return;
	token_litstr(); if (try_token(L_STR)) return;
	token_varname(); if (try_token(VNAME)) return;
	token_funname(); if (try_token(FNAME)) return;
	token_moperator(); if (try_token(OP_MATH)) return;
	
	lex_error("\nInvalid token: ");
}

int lex_sep(struct Token token) {
	int s = buff_distance(token.lexeme_begin, token.lexeme_end);
	int c = token.column;
	return s + c/TAB_SIZE;
}

void lex_print() {
	printf("%u:%u\t", token.line, token.column);
		
	//Special case for TABS
	if (token.id == SEP) {
		int s = buff_distance(token.lexeme_begin, token.lexeme_end);
		int c = token.column;
		int n = s + c/TAB_SIZE;
		printf("<SEP, %u>\n", n);
		return;
	}
	
	//Special cases
	if (token.id == NL) 	{ printf("<NL>\n"); return; }
	if (token.id == SPACE) 	{ printf("<SPACE>\n"); return; }
	
	//ID to name translation
	char* name;
	switch (token.id) {
		case L_BOOL: name = "L_BOOL"; break;
		case L_STR: name = "L_STR"; break;
		case L_NUM: name = "L_NUM"; break;
		case VNAME: name = "VNAME"; break;
		case FNAME: name = "FNAME"; break;
		default: name = "OPERATOR";
	}
	
	printf("<%s, ", name); fflush(stdout);
	buff_print(token.lexeme_begin, token.lexeme_end);
	printf(">\n"); fflush(stdout);
}

void lex_init() {
	line = 1;
	count = 0;
	sep_flag = FALSE;
	eof_flag = FALSE;
	lexeme_begin = buff_init();
	forward = lexeme_begin;
}

void lex_next() {
	if (*lexeme_begin == '\0') {
		if (!eof_flag) { eof_flag = TRUE; token.id = NL; return; }
		return;
	}
	
	//Handle comments
	if (*lexeme_begin == '#') {
		while (*lexeme_begin == '#') {
			while (*lexeme_begin != '\0') {
				token_newline();
				if (try_token(NL)) {
					lexeme_begin = forward;
					break;
				}
				forward = buff_next();
				lexeme_begin = forward;
			}
			line++;
			count = buff_get_count();
			sep_flag = FALSE;
		}
	} else {
		match();
	}
	
	token.line = line;
	token.column = buff_get_count() - count;
	token.lexeme_begin = lexeme_begin;
	token.lexeme_end = forward;
	lexeme_begin = forward;
}