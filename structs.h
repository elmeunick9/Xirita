/*
This file includes basic datastructures on a Var object wich can hold all Xirita variable types. This datastructures include:
	- List (as Linked-List)
	- Dictionary (as Hash Table)
*/

#ifndef XIRITA_HASH_H
#define XIRITA_HASH_H

#include <stdlib.h>

struct List;
struct Pair;
struct ASTNode;

typedef struct {
	enum VARType { NUM, STR, BOOL, LIST, DICT, PAIR, ASTNODE, FUNCTION } type;
	union {
		long int as_int;
		char* as_str;
		unsigned char as_bool;
		struct List* as_list;
		struct Dict* as_dict;
		struct Pair* as_pair;
		struct ASTNode* as_ast;
		void (*as_func)();
	};
} Var;

enum ASTType { AST_CALL, AST_DECL, AST_LINE, AST_BLOCK, AST_VAR, AST_VAL };
struct ASTNode {
	struct ASTNode* right;
	struct ASTNode* down;
	enum ASTType type;
	Var value;
};

struct ListNode {
	struct ListNode* next;
	Var value;
};

struct List {
	size_t size;
	struct ListNode* begin;
};

struct Pair {
	char* key;
	Var value;
};

struct Dict {
	size_t size;
	size_t allocated;
	struct List* nodes; // Shall be PAIRS
};

char* makestring(char* x);

Var makeVar();
Var makeNum(int x);
Var makeBool(unsigned int x);
Var makeStr(char* x);
Var makePair(char* key, Var val);
Var makeFunction(void (*ptr)());
Var makeNode(struct ASTNode* node);
Var makeList(struct List* list);
Var makeDict(struct Dict* dict);
void printVar(Var x);
void freeVar(Var x);
Var copyVar(Var x);


struct ASTNode* ast_new(enum ASTType t, Var value);
void 			ast_print(struct ASTNode* ast);
void 			ast_print_node(struct ASTNode* ast);
void 			ast_clear(struct ASTNode* ast);

struct List* 	list_new();
void 			list_add(struct List* list, size_t i, Var value);	//Index is position where the added element will be.
Var 			list_get(const struct List* list, size_t i);		//May produce OutOfBounds error.
void			list_del(struct List* list, size_t i);				//Frees memory.
void 			list_set(struct List* list, size_t i, Var value);
void			list_print(const struct List*);
void			list_clear(struct List*);							//Deletes all elements in list.

struct Dict*	dict_new();
void			dict_add(struct Dict* dict, char* key, Var value);	//Adds element or modifies if already exists.
unsigned char	dict_find(const struct Dict*, char* key);			//Returns whether element exists or not.
Var				dict_get(const struct Dict* dict, char* key);		//Returns value for a given key, returns default if not exists.
void			dict_del(struct Dict* dict, char* key);				//Deletes key,value pair.
void			dict_clear(struct Dict*);							//Deletes all elements in dictionary.
void			dict_print(const struct Dict*);

//Testing
void structs_test();

#endif