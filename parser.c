#include "utils.h"
#include "structs.h"
#include "sdt_run.h"

//This is a LL(1) Recursive-Descent Parser

/* --- Grammar ---
program -> (line NL | NL )*
line  -> rline | SEP rline | VNAME SEP rline
rline -> FNAME (SPACE FNAME|arg)* (:)? | fdecl (SPACE rline)?
arg -> VNAME | L_STR | L_NUM | L_BOOL
fdecl -> OFB FNAME (SPACE FNAME|arg)* CFB
*/

bool fail = FALSE;
struct List* sepstack = NULL;
char* lexeme = NULL;
char* last_seen_vname = "";
struct ASTNode* root = NULL;
struct ASTNode* node = NULL;

extern Var makeVar();
extern Var makeNum();
extern Var makeStr();
extern Var makeBool();
extern Var makeNode();
extern bool eof_flag;

void syntax_error() {
	printf("Syntax error on token: "); fflush(stdout);
	lex_print();
	exit(1);
}

static void right(enum ASTType t, Var v) {
	node->right = ast_new(t, v); node = node->right;
}

static void down(enum ASTType t, Var v) {
	node->down = ast_new(t, v); node = node->down;
}

static void match(enum TokenID id) {
	if (id != token.id) fail = TRUE;
	else {
		if (lexeme) free(lexeme);
		lexeme = buff_copy(token.lexeme_begin, token.lexeme_end);
		lex_next();
	}
}

static struct ASTNode* getBlockLine(struct Token sep) {
	int level = lex_sep(sep);
	while (level < sepstack->size-1) list_del(sepstack, 0);
	return list_get(sepstack, 0).as_ast;
}

//------------------------------------------------------
static void fdecl();
static void arg() {
	switch (token.id) {
		case VNAME:
			match(VNAME);
			right(AST_VAR, makeStr(lexeme));
			break; 
		case L_NUM:
			match(L_NUM);
			int num = strtol(lexeme, NULL, 10);
			right(AST_VAL, makeNum(num));
			break;
		case L_STR:
			match(L_STR);
			char* word = lexeme+1;
			word[strlen(word)-1]='\0';
			right(AST_VAL, makeStr(word));
			break;
		case L_BOOL:
			match(L_BOOL);
			unsigned char boo = (*lexeme == 'T') ? 1 : 0;
			right(AST_VAL, makeBool(boo));
			break;
	}
}

static void rline() {
	struct ASTNode* root = node;
	switch(token.id) {
		case FNAME:
			match(FNAME);		right(AST_CALL, makeStr(lexeme));
			while(!fail) {
				match(SPACE);
				if (token.id == FNAME) {
					match(FNAME); right(AST_CALL, makeStr(lexeme));
				}
				else arg();
			}
			fail = FALSE;
			node->right = NULL;
			node->down = NULL;
			
			//-----------------
			
			if (token.id == OP_BLOCK) {
				node = root->right;
				match(OP_BLOCK);
				down(AST_LINE, makeStr("%SBLOCKLINE%"));
				if (token.id != SPACE) {
					right(AST_LINE, makeStr("%LINE%"));
					list_add(sepstack, 0, makeNode(node));
				} else {
					match(SPACE);
					right(AST_LINE, makeStr("%LINE%"));
					list_add(sepstack, 0, makeNode(node));
					right(AST_VAR, makeStr(last_seen_vname));
					rline();
					list_del(sepstack, 0);
				}
			}
			else if (token.id == OP_INLINE) {
				node = list_get(sepstack, 0).as_ast;
				match(OP_INLINE); match(SPACE);
				down(AST_LINE, makeStr("%LINE%"));
				list_set(sepstack, 0, makeNode(node));
				right(AST_VAR, makeStr(last_seen_vname));
				rline();
			}
			
			break;
		default: fdecl();		
	}
	node = root;
}

static void fdecl() {
	static char buff[1024];
	char* signature = buff;
	match(OFB);
	match(FNAME);
	struct ASTNode* t = node;
	right(AST_DECL, makeStr(buff)); 
	down(AST_LINE, makeStr("%ARGLIST%"));
	node->down = NULL;
	node->right = NULL;
	
	strcpy(signature, lexeme);
	while(token.id != CFB) {
		match(SPACE);
		struct ASTNode* t = node;
		switch(token.id) {
			case FNAME:
				match(FNAME);
				
				strcat(signature, "_");
				strcat(signature, lexeme);
				break;
			default:
				arg();
				strcat(signature, sdt_code_litsignature(node));
		}
				
	}
	node = t;
	node->right->value=makeStr(buff); node = node->right;
	
	match(CFB);
	if (token.id == SPACE) {
		struct ASTNode* t = node;
		right(AST_LINE, makeStr("%LINE%"));
		list_add(sepstack, 0, makeNode(node));
		right(AST_VAR, makeStr(""));
		match(SPACE);
		rline();
		list_del(sepstack, 0);
		node = t;
	} else {
		right(AST_LINE, makeStr("%LINE%"));
		list_add(sepstack, 0, makeNode(node));
	}
}

static void line() {
	unsigned char sep = FALSE;
	static char buff[64];
	buff[0] = '\0';
	char* vname = buff;
	switch (token.id) {
		case VNAME: match(VNAME); 	strncpy(vname, lexeme, 62); last_seen_vname = vname;
		case SEP:	node = getBlockLine(token); sep = TRUE; match(SEP);		
		default:
			if (!sep) {
				while(sepstack->size > 1) list_del(sepstack, 0);
				node = list_get(sepstack, 0).as_ast;
			}
			if (node==root) {
				root->right = ast_new(AST_LINE, makeStr("%LINE%"));
				node = root->right; }
			else {
				down(AST_LINE, makeStr("%LINE%"));
			}
			list_set(sepstack, 0, makeNode(node));
			right(AST_VAR, makeStr(vname));
			rline();
	}
	node = list_get(sepstack, 0).as_ast;
}

static void program() {
	fail = FALSE;
	lexeme = NULL;
	last_seen_vname = "";
	root = ast_new(AST_DECL, makeStr("%ROOT%"));
	node = root;
	sepstack = list_new();
	list_add(sepstack, 0, makeNode(node));
	
	while(!fail) {
		switch (token.id) {
			case NL: match(NL); break;
			default: line(); match(NL);
		}
		if (eof_flag) break;
	}
	if (fail) syntax_error();
	list_clear(sepstack);
	free(sepstack);
}

void run_lexer() {
	lex_init();
	while(!eof_flag) {
		lex_next();
		lex_print();
	}
}

void run_parser() {
	//printf("--PARSING--\n"); fflush(stdout);
	lex_init();
	lex_next();
	program(); //Compile into an AST
}

void run_program() {
	//printf("--EXECUTION--\n"); fflush(stdout);
	sdt_init();
	sdt_prerun(root, "");
	sdt_run(root);
	sdt_run_main();
}