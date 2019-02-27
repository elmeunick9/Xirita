#include "structs.h"
#include <stdio.h>
#include <string.h>

#define INDEX_OUT_OF_BOUNDS_ERROR() { fprintf(stderr, "[ERROR] (%s:%d) Index out of bounds.\n", __FILE__, __LINE__); exit(1); }

typedef struct List List;
typedef struct ListNode ListNode;
typedef struct Dict Dict;
typedef struct Pair Pair;

//Var Policy is to never use references, it always copies averything. As such, freeing a variable will never mess up other variables. There are no cross pointer references.

char* makestring(char* x) {
	char* n = (char*) malloc(sizeof(char)*strlen(x)+1);
	strcpy(n, x); return n;
}

Var makeVar() { Var n; n.type = NUM; n.as_int = 0; return n; }
Var makeNum(int x) { Var n; n.type = NUM; n.as_int = x; return n; }
Var makeBool(unsigned int x) {
	Var n; n.type = BOOL;
	n.as_bool = (x > 0) ? 1 : 0;
	return n;
}
Var makeStr(char* x)	{
	Var n; n.type = STR;
	n.as_str = makestring(x);
	return n;
}
Var makePair(char* key, Var val) {
	Var n; n.type = PAIR;
	n.as_pair = (Pair*) calloc(1, sizeof(Pair));
	n.as_pair->key = makestring(key);
	n.as_pair->value = val;
	return n;
}
Var makeFunction(void (*ptr)()) {
	Var n; n.type = FUNCTION;
	n.as_func = ptr;
	return n;
}
Var makeNode(struct ASTNode* node) {
	Var n; n.type = ASTNODE;
	n.as_ast = node;
	return n;
}
Var makeList(struct List* list) {
	Var n; n.type = LIST;
	n.as_list = list;
	return n;
}
Var makeDict(struct Dict* dict) {
	Var n; n.type = DICT;
	n.as_dict = dict;
	return n;
}
void printVar(Var x) {
	switch(x.type) {
		case NUM: printf("%i", x.as_int); break;
		case STR: printf("%s", x.as_str); break;
		case BOOL: printf(x.as_bool ? "True" : "False"); break;
		case PAIR: printf("<%s,", x.as_pair->key); printVar(x.as_pair->value); printf(">"); break;
		case LIST: list_print(x.as_list);
	}
}
void freeVar(Var x) {
	switch(x.type) {
		case STR: free(x.as_str); break;
		case PAIR: free(x.as_pair->key); freeVar(x.as_pair->value); free(x.as_pair); break; 
	}
}
Var copyVar(Var x) {
	Var n; n = x;
	switch(x.type) {
		case STR: n.as_str = makestring(x.as_str); break;
		case PAIR: n.as_pair->key = makestring(x.as_pair->key); n.as_pair->value = copyVar(x.as_pair->value); break; 
	}
	return n;
}

//---------------------------------- LISTS ------------------------------------

struct List* list_new() {
	List* list = (List*) malloc(sizeof(List));
	list->size = 0;
	list->begin = NULL;
	return list;
}

static ListNode* list_get_node(const List* list, size_t i) {
	if (i >= list->size) return NULL;
	size_t p = 0;
	struct ListNode* node = list->begin; 
	while(p < i) { 
		node = node->next;
		p++;
	}
	return node;
}
Var list_get(const List* list, size_t i) {
	if (i >= list->size) INDEX_OUT_OF_BOUNDS_ERROR();
	struct ListNode* node = list_get_node(list, i);
	if (node == NULL) { printf("NULL!"); exit(0); }
	return node->value;
}

void list_add(List* list, size_t i, Var value) {
	if (i > list->size) INDEX_OUT_OF_BOUNDS_ERROR();
	ListNode* left = list_get_node(list, i-1);
	ListNode* middle = (ListNode*) malloc(sizeof(ListNode));
	middle->value = value;
	middle->next = NULL;
	
	if ((int) i-1 < 0) {
		ListNode* rigth = list->begin;
		list->begin = middle;
		middle->next = rigth;
	} else {
		if (left == NULL) INDEX_OUT_OF_BOUNDS_ERROR();
		ListNode* rigth = left->next;
		left->next = middle;
		middle->next = rigth;
	}
	
	list->size++;
	//return &(middle->value);
}
void list_set(List* list, size_t i, Var value) {
	ListNode* n = list_get_node(list, i);
	freeVar(n->value);
	n->value = value;
}

void list_del(List* list, size_t i) {
	if (i >= list->size) INDEX_OUT_OF_BOUNDS_ERROR();
	
	if ((int) i-1 < 0) {
		ListNode* rigth = list->begin->next;
		freeVar(list->begin->value);
		free(list->begin);
		list->begin = rigth;
	} else {
		ListNode* left = list_get_node(list, i-1);
		ListNode* middle = left->next;
		ListNode* rigth = middle->next;
		freeVar(middle->value);
		free(middle);
		left->next = rigth;
	}
	list->size--;
}

void list_print(const struct List* list) {
	int i = 0;
	printf("[");
	for (i; i<list->size; i++) {
		printVar(list_get(list, i));
		if (i != list->size-1) printf(", ");
	}
	printf("]\n");
}

void list_clear(struct List* list) {
	while(list->size > 0) list_del(list, 0);
}

//---------------------------------- DICTS ------------------------------------

static unsigned long hash(char* str) {
	unsigned long c = 0;
	int i = 0;
	while(*str) {
		c += i*41+(int) *str;
		str++;
		i++;
	}
	return c;
}

static Dict* dict_new_size(size_t n);
static void dict_realloc(Dict* dict, size_t n) {
	if (n == 0) return;
	Dict* dcopy = dict_new_size(n);
	size_t i = 0;
	for (i; i<dict->allocated; i++) {
		size_t j = 0;
		for(j; j<dict->nodes[i].size; j++) {
			Var pair = list_get(dict->nodes+i, j);
			char* key = pair.as_pair->key;
			Var value = pair.as_pair->value;
			size_t p = (size_t) hash(key) % dcopy->allocated;
			list_add(dcopy->nodes+p, dcopy->nodes[p].size, pair);
		}
	}
	free(dict->nodes);
	dict->nodes = dcopy->nodes;
	free(dcopy);
	dict->allocated = n;
	//printf("REALLOC ALLOCATED: %u SIZE: %u\n", dict->allocated, dict->size); fflush(stdout);
}

static void dict_memcheck(Dict* dict) {
	if (dict->size<32) return;
	if (dict->allocated > 8*dict->size) {
		dict_realloc(dict, dict->allocated/2);
	}
	if (dict->allocated == 2*dict->size) {
		dict_realloc(dict, dict->allocated*2);
	}
}

static Dict* dict_new_size(size_t n) {
	Dict* dict = (Dict*) malloc(sizeof(Dict));
	dict->size = 0;
	dict->nodes = (List*) calloc(n, sizeof(List));
	dict->allocated = n;
	return dict;
}

Dict* dict_new() {
	return dict_new_size(64);
}

static ListNode* dict_get_node(const struct Dict* dict, char* key, size_t* index) {
	size_t p = (size_t) hash(key) % dict->allocated;
	List* ls = dict->nodes+p;
	size_t i = 0;
	for (i; i<ls->size;i++) {
		ListNode* node = list_get_node(ls, i);
		if (strcmp(node->value.as_pair->key, key) == 0) {
			if (index != NULL) *index = i;
			return node;
		}
	}
	return NULL;
}	

void dict_add(struct Dict* dict, char* key, Var value) {
	dict_memcheck(dict);
	ListNode* node = dict_get_node(dict, key, NULL);
	if (node == NULL) {
		dict->size++;
		size_t p = (size_t) hash(key) % dict->allocated;
		list_add(dict->nodes+p, 0, makePair(key, value));		
	} else {
		node->value.as_pair->value = value;
	}
}

unsigned char dict_find(const struct Dict* dict, char* key) {
	return (dict_get_node(dict, key, NULL) != NULL);
}

Var	dict_get(const struct Dict* dict, char* key) {
	ListNode* node = dict_get_node(dict, key, NULL);
	if (node == NULL) return makeVar();
	else return node->value.as_pair->value;
}

void dict_print(const struct Dict* dict) {
	printf("{");
	unsigned char first = 1;
	size_t i; for (i=0; i<dict->allocated; i++) {
		ListNode* ln = dict->nodes[i].begin;
		while(ln) {
			if (!first) printf(", ");
			else first = 0;
			printf("%s: ", ln->value.as_pair->key);
			printVar(ln->value.as_pair->value);
			ln = ln->next;
		}
	}
	printf("}\n");
}

void dict_clear(struct Dict* dict) {
	size_t i; for (i=0; i<dict->allocated; i++) {
		list_clear(dict->nodes+i);
	}
	dict->size = 0;
	dict_memcheck(dict);
}

void dict_del(struct Dict* dict, char* key) {
	size_t i;
	ListNode* node = dict_get_node(dict, key, &i);
	if (node == NULL) return;
	size_t p = (size_t) hash(key) % dict->allocated;
	List* ls = dict->nodes+p;
	list_del(ls, i);
	dict->size--;
	dict_memcheck(dict);
}


void structs_test() {
	List* list = list_new();
	list_add(list, 0, makeStr("Hello"));
	list_add(list, 0, makeStr("World"));
	list_add(list, 0, makeStr("And"));
	list_add(list, 0, makeStr("So"));
	list_add(list, 0, makeStr("On"));
	list_del(list, 2);
	
	Dict* dict = dict_new();
	dict_add(dict, "Hello", makeNum(1));
	dict_add(dict, "LURS", makeNum(2));
	dict_add(dict, "Hello", makeNum(3));
	dict_add(dict, "mmm", makeNum(4));
	dict_add(dict, "aaa", makeNum(5));
	dict_add(dict, "bbb", makeNum(6));
	dict_add(dict, "bbb", makeNum(7));
	dict_add(dict, "bba", makeNum(8));
	dict_add(dict, "bbc", makeNum(9));
	dict_add(dict, "bbd", makeNum(0));
	dict_add(dict, "bsd", makeNum(0));
	dict_del(dict, "bbb");
	dict_del(dict, "bbb");
	dict_del(dict, "bbd");
	dict_del(dict, "aaa");
	dict_del(dict, "Hello");
	dict_del(dict, "LURS");
	dict_del(dict, "mmm");
	dict_print(dict);
};