#include "structs.h"
#include <stdio.h>

struct ASTNode* ast_new(enum ASTType t, Var value) {
	struct ASTNode* node = (struct ASTNode*) malloc(sizeof(struct ASTNode));
	node->right = NULL;
	node->down = NULL;
	node->type = t;
	node->value = value;
	return node;
}

extern void printVar();
void ast_print(struct ASTNode* ast) {
	while(ast) {
		if (ast->type == AST_LINE)	{ printf("NEW LINE\n"); }
		if (ast->type == AST_VAL) 	{ printVar(ast->value); printf(" "); }
		if (ast->down) 				{ printf("\n"); ast_print(ast->down); }
		ast = ast->right;
	}
}

void ast_print_node(struct ASTNode* node) {
	if (node == NULL) { printf("(NULL)"); fflush(stdout); return; }
	switch(node->type) {
		case AST_LINE: printf("<LINE>"); break;
		case AST_VAL: printf("<VAL: "); fflush(stdout); printVar(node->value); printf(">"); break;
		case AST_VAR: printf("<VAR: "); fflush(stdout); printVar(node->value); printf(">"); break;
		case AST_CALL: printf("<CALL: "); fflush(stdout); printVar(node->value); printf(">"); break;
		default: printf("<>");
	}
	fflush(stdout);
}

void ast_clear(struct ASTNode* ast) {
	printf("AST CLEAR NOT IMPLENTED!\n");
}