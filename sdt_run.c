#include "sdt_run.h"
#include "structs.h"
#include "utils.h"

#include <stdio.h>

struct Dict* enviorment = NULL;
struct Dict* fmap = NULL;
struct Dict* imports = NULL;
struct List* arglist = NULL;
static char* variable = NULL;
static struct ASTNode* node = NULL;
static struct ASTNode* linenode = NULL;
extern struct ASTNode* root; //From parser.c
static struct ASTNode* p_return = NULL;

static void runtime_error(char* msg) { 
	printf("ERROR: %s\n", msg); exit(1);
}

static Var getVariable(char* name, enum VARType t) {
	Var x = dict_get(enviorment, name);
	if (x.type != t) {
		switch(t) {
			case NUM: runtime_error("Requieres variable of type number.");
			case STR: runtime_error("Requieres variable of type string.");
			case BOOL: runtime_error("Requieres variable of type bool.");
			case LIST: runtime_error("Requieres variable of type list.");
			default: runtime_error("Requieres variable of incopatible type. Something is worng with the compiler!");
		}
	}
	return x;
}

//------- Predefined Functions: Control ---------
static void p_assign() {
	if (arglist->size == 0) runtime_error("ASSIGN requieres at least 1 argument.");
	if (arglist->size == 1) {
		dict_add(enviorment, variable, copyVar(list_get(arglist, 0)));
	}
}

static void p_print() {
	printVar(dict_get(enviorment, variable)); printf("\n"); fflush(stdout);
}

static void p_while() {
	Var arg0 = getVariable(variable, BOOL);
	struct ASTNode* home = node->down;
	char* var0 = makestring(variable);
	while(arg0.as_bool) {
		sdt_run(home);
		arg0 = getVariable(var0, BOOL);
	}
	free(var0);
}

static void p_block() {
	struct ASTNode* node = dict_get(enviorment, "_block").as_ast;
	struct Dict* env = enviorment;
	char* vname = dict_get(enviorment, "_blockvname").as_str;
	enviorment = dict_get(enviorment, "_parentenv").as_dict;
	dict_add(enviorment, vname, copyVar(dict_get(env, variable)));
	vname = variable;
	
	sdt_run(node);
	
	dict_add(env, vname, copyVar(dict_get(enviorment, variable)));
	enviorment = env;
	variable = vname;
}

extern unsigned int script_fileno;
extern void run_parser();
static void p_import() {
	if (arglist->size != 1) runtime_error("IMPORT requieres 1 argument.");
	Var arg1 = list_get(arglist, 0);
	if (arg1.type != STR) runtime_error("IMPORT argument must be of type string.");
	
	char* script = arg1.as_str;
	if (dict_find(imports, script)) {
		root = dict_get(imports, script).as_ast;
	}
	else {
		FILE *in_file  = fopen(script, "r");
		if (in_file == NULL) {
			fprintf(stderr, "Can't open input file for import (%s).\n", script);
			exit(1);
		}
		
		script_fileno = fileno(in_file);
		run_parser(); //root modified here.
		fclose(in_file);
		dict_add(imports, script, makeNode(root));
	}
	
	printf("IMPORT %s WITH PREFIX %s\n", script, dict_get(enviorment, "_importprefix").as_str);
	sdt_prerun(root, dict_get(enviorment, "_importprefix").as_str);
	sdt_run(root);
}

static void p_argument() {
	if (arglist->size != 1) runtime_error("ARGUMENT requieres 1 argument.");
	Var arg1 = list_get(arglist, 0);
	if (arg1.type != NUM) runtime_error("ARGUMENT argument must be of type number.");
	size_t index = arg1.as_int;
	Var argx = list_get(dict_get(enviorment, "_arguments").as_list, index);
	dict_add(enviorment, variable, argx);
}

//------- Predefined Functions: Logic ---------
static void pp_then(char* variable, Var arg0) {
	char* var0 = makestring(variable);
	if (arglist->size != 0) runtime_error("THEN requieres no arguments.");
	if (arg0.as_bool) sdt_run(node->down);
	//dict_add(enviorment, var0, arg0);
	free(var0);
}
static void p_then() {
	Var arg0 = getVariable(variable, BOOL);
	pp_then(variable, arg0);
}
static void p_else() {
	Var arg0 = getVariable(variable, BOOL);
	pp_then(variable, makeBool(!arg0.as_bool));
}
static void p_not() {
	Var arg0 = getVariable(variable, BOOL);
	dict_add(enviorment, variable, makeBool(!arg0.as_bool));
}
static void p_eq() {
	Var arg0 = dict_get(enviorment, variable);
	if (arglist->size != 1) runtime_error("EQ requieres 1 argument only.");
	Var arg1 = list_get(arglist, 0);
	unsigned char eq = FALSE;
	if (arg0.type == arg1.type) {
		switch(arg1.type) {
			case NUM: eq = (arg0.as_int == arg1.as_int); break;
			case BOOL: eq = (arg0.as_bool == arg1.as_bool); break;
			case STR:
				if (strlen(arg0.as_str) != strlen(arg1.as_str)) break;
				eq = (strcmp(arg0.as_str, arg1.as_str) == 0);
			//TODO: Lists & functions
		}
	}
	dict_add(enviorment, variable, makeBool(eq));
}

//------- Predefined Functions: Math ---------
static void p_add() {
	if (arglist->size != 1) runtime_error("ADD requieres 1 argument only.");
	Var arg0 = getVariable(variable, NUM);
	Var arg1 = list_get(arglist, 0); 
	if (arg1.type != NUM) runtime_error("ADD arguments must be of type NUMBER");
	dict_add(enviorment, variable, makeNum(arg0.as_int + arg1.as_int));
}
static void p_sub() {
	if (arglist->size != 1) runtime_error("SUB requieres 1 argument only.");
	Var arg0 = getVariable(variable, NUM);
	Var arg1 = list_get(arglist, 0); 
	if (arg1.type != NUM) runtime_error("SUB arguments must be of type NUMBER");
	dict_add(enviorment, variable, makeNum(arg0.as_int - arg1.as_int));
}


//------- Predefined Functions: Lists --------
static void p_list() {
	size_t i = 0;
	struct List* list = list_new();
	for(i;i<arglist->size;i++) {
		list_add(list, i, list_get(arglist, i));
	}
	dict_add(enviorment, variable, makeList(list));
}

static void p_list_get() {
	Var arg1 = list_get(arglist, 0);
	Var arg2 = list_get(arglist, 1);
	if (arg1.type != LIST) runtime_error("LIST GET argument 0 must be of type LIST");
	if (arg2.type != NUM) runtime_error("LIST GET argument 1 must be of type NUMBER");
	dict_add(enviorment, variable, list_get(arg1.as_list, arg2.as_int-1));
}

static void p_list_set() {
	Var arg0 = dict_get(enviorment, variable);
	Var arg1 = list_get(arglist, 0);
	Var arg2 = list_get(arglist, 1);
	if (arg1.type != LIST) runtime_error("LIST SET argument 0 must be of type LIST");
	if (arg2.type != NUM) runtime_error("LIST SET argument 1 must be of type NUMBER");
	list_set(arg1.as_list, arg2.as_int-1, arg0);
}

static void p_list_size() {
	Var arg1 = list_get(arglist, 0);
	if (arg1.type != LIST) runtime_error("LIST SIZE argument 0 must be of type LIST");
	dict_add(enviorment, variable, makeNum(arg1.as_list->size));
}

//---------- Virtual Machine -----------

void sdt_init() {
	enviorment = dict_new();
	fmap = dict_new();
	imports = dict_new();
	
	dict_add(fmap, "ASSIGNv", makeFunction(&p_assign));
	dict_add(fmap, "PRINT", makeFunction(&p_print));
	dict_add(fmap, "ARGUMENTv", makeFunction(&p_argument));
	dict_add(fmap, "WHILE", makeFunction(&p_while));
	dict_add(fmap, "BLOCK", makeFunction(&p_block));
	dict_add(fmap, "IMPORTv", makeFunction(&p_import));
	
	dict_add(fmap, "THEN", makeFunction(&p_then));
	dict_add(fmap, "ELSE", makeFunction(&p_else));
	dict_add(fmap, "NOT", makeFunction(&p_not));
	dict_add(fmap, "EQv", makeFunction(&p_eq));
	
	dict_add(fmap, "ADDv", makeFunction(&p_add));
	dict_add(fmap, "SUBv", makeFunction(&p_sub));
	
	dict_add(fmap, "LIST", makeFunction(&p_list));
	dict_add(fmap, "LISTv_GETv", makeFunction(&p_list_get));
	dict_add(fmap, "LISTv_SETv", makeFunction(&p_list_set));
	dict_add(fmap, "LISTv_SIZE", makeFunction(&p_list_size));
	
	dict_add(enviorment, "_declprefix", makeStr(""));
}

static void sdt_filluserargs(struct ASTNode* function, struct List* arglist) {
	struct ASTNode* arg = function->down->right;
	size_t i = 0;
	for(;arg;arg=arg->right,i++) {
		if (arg->type != AST_VAR) continue;
		dict_add(enviorment, arg->value.as_str, list_get(arglist, i));
	}
}

static char* signature_strip_arguments(char* signature) {
	static char buff[1024];
	strcpy(buff, signature);
	size_t i = strlen(signature);
	for(i; i>0; i--) {
		if(buff[i-1] != 'v') {
			buff[i] = '\0';
			break;
		}
	}
	return buff;
}

static Var sdt_resolve_function(char** xfname) {
	char* fname = *xfname;
	char* prefix = dict_get(enviorment, "_declprefix").as_str;
	static char buff[1024];
	char* name = buff;
	
	//Try on diferent namespaces (treating arguments as namespace separators)
	//printf("RESOLVE: %s_%s\n", prefix, fname); fflush(stdout);
	Var f;
	size_t i = strlen(prefix);
	for(i; i>0; i--) {
		strncpy(name, prefix, i);
		name[i] = '\0';
		strcat(name, "_");
		strcat(name, fname);
		f = dict_get(fmap, name);
		if (f.as_int != 0) {
			//printf("RESOLVE FOUND: %s\n", name); fflush(stdout);
			*xfname = name; return f;
		}
		
		f = dict_get(fmap, signature_strip_arguments(name));
		//printf("RESOLVE TRY: %s\n", signature_strip_arguments(name)); fflush(stdout);
		if (f.as_int != 0) {
			//printf("RESOLVE FOUND: %s\n", name); fflush(stdout);
			*xfname = name; return f;
		}
		
		while(i>1 && prefix[i-1] != '_' && prefix[i-1] != 'v' && prefix[i-1] != 'l') i--;
		//printf("RESOLVE FAILED WITH: %s\n", name); fflush(stdout);
	}
	
	//Try generic signature directly
	f = dict_get(fmap, fname);
	if (f.as_int != 0) {
		//printf("RESOLVE FOUND: %s\n", fname); fflush(stdout);
		return f;
	}
	
	//printf("RESOLVE TRY: %s\n", signature_strip_arguments(fname)); fflush(stdout);
	f = dict_get(fmap, signature_strip_arguments(fname));
	if (f.as_int != 0) {
		//printf("RESOLVE FOUND: %s\n", name); fflush(stdout);
		*xfname = name; return f;
	}
	
	printf("RuntimeError: Function %s not found!\n", fname); exit(1);
	return f;
}

/*
This may requiere some explanation. The following function resolves generic signatures into a node of a specific signature by applying simple pattern matching on the arguments of the declaration and call.

For any generic signature, a list of pairs (map) is given with specific signatures as keys and nodes as values. We loop the whole list and select the most appropiate match. Then return. The following rules apply:
	- Only functions with the same generic signature match, that is, same number of arguments in same positions.
	- Only functions that match in all their literals (declaration) match.
	- The function with the most number of literals matched is the one selected.
	- If multiple functions are selected with the same number of literals, the last one declared is selected, example:
		[F a 2]
		[F 1 b]
		F 1 2 #Calls [F 1 b]
*/
static Var sdt_resolve_pattern_matching(struct List* map) {
	static char buff[1024];
	size_t 			selected = 0;
	unsigned char	matched  = 0;
	
	size_t i = 0;
	for(i; i<map->size ; i++) {
		char* signature = list_get(map, i).as_pair->key;
		char* p = signature;
		size_t j = 0;
		unsigned char match = 0;
		unsigned char unmatch = FALSE;
		while(*p && unmatch == FALSE) {
			if (*p == 'l') {
				p++;
				
				char* t = p;
				while(*t != '$') t++;
				t--;
				
				strncpy(buff, p+1, t-p);
				buff[t-p] = '\0'; 			//Contains the argument in the signature as string
				
				Var arg = list_get(arglist, j);
				//TODO: The following code duplicates that of parser.c arg() 
				switch(*p) {
					case 'i':
						p++;
						if (arg.type != NUM) { unmatch = TRUE; break; }
						long int num = strtol(buff, NULL, 10);
						if (num == arg.as_int) match++;
						else unmatch = TRUE;
						break;						
				}
				j++;
				
				while(*p != '$') p++;
			}
			p++;
		}
		
		if (match >= matched && !unmatch) { matched = match; selected = i; }
		
	}
	
	return list_get(map, selected).as_pair->value;
}

static void sdt_call(char* vname, char* fname, struct List* local_arglist, struct ASTNode* local_node) {
	//printf("%s %s", vname, fname); list_print(local_arglist); fflush(stdout);
	node = local_node;
	arglist = local_arglist;
	Var f = sdt_resolve_function(&fname);

	variable = vname;
	if (f.type == FUNCTION) (*f.as_func)();
	if (f.type == LIST) {
		f = sdt_resolve_pattern_matching(f.as_list);
		struct Dict* env = enviorment;
		char* var0 = makestring(variable);
		
		enviorment = dict_new();
		dict_add(enviorment, "_arguments", makeList(arglist));
		dict_add(enviorment, "_block", makeNode(node->down));
		dict_add(enviorment, "_blockvname", makeStr(variable));
		dict_add(enviorment, "_parentenv", makeDict(env));
		dict_add(enviorment, "_declprefix", makeStr(fname));
		dict_add(enviorment, "_declast", makeNode(f.as_ast));
		dict_add(enviorment, "", copyVar(dict_get(env, variable)));
		sdt_filluserargs(f.as_ast, arglist);
		sdt_run(f.as_ast);
		dict_add(env, var0, copyVar(dict_get(enviorment, variable)));
		
		free(var0);
		dict_clear(enviorment);
		free(enviorment);
		enviorment = env;
		if (p_return == f.as_ast) p_return = NULL; 
	}
}

static char* sdt_signature_generic(char* fname) {
	static char buff[1024];
	char* p = buff;
	strncpy(p, fname, 1024);
	while(*p) {
		if (*p == 'l') {
			*p = 'v';
			p++;
			char* t = p;
			while(*t != '$') t++;
			strcpy(p, ++t);
		}
		p++;
	}
	return buff;
}

static void sdt_decl(char* vname, char* fname, struct ASTNode* node) {
	char* prefix = dict_get(enviorment, "_declprefix").as_str;
	strcat(fname, prefix); //No allocation only becouse fname is buff[1024]!
	
	//printf("DECLARATION %s FOUND: (Generic) %s\n", fname, sdt_signature_generic(fname)); fflush(stdout);
	char* generic = sdt_signature_generic(fname);
	if (!dict_find(fmap, generic)) {
		struct List* list = list_new();
		list_add(list, list->size, makePair(fname, makeNode(node)));
		dict_add(fmap, generic, makeList(list));
	} else {
		struct List* list = dict_get(fmap, generic).as_list;
		list_add(list, list->size, makePair(fname, makeNode(node)));
		dict_add(fmap, generic, makeList(list));
	}
}

char* sdt_code_litsignature(const struct ASTNode* node) {
	static char buff[255];
	sprintf(buff, "l(null)$");
	if (node == NULL) return buff;
	
	Var x = node->value;
	if (node->type == AST_VAL) {
		switch(x.type) {
			case NUM:
				sprintf(buff, "li%u$", x.as_int);
		}
	} else if (node->type == AST_VAR) {
		sprintf(buff, "v");
	} else {
		sprintf(buff, "_%s", node->value.as_str);
	}
	
	return buff;
}

void sdt_prerun(struct ASTNode* root, char* prefix) {
	//printf("(prerun) ROOT: %s\n", root->value.as_str); fflush(stdout);
	char buff[1024];
	char* signature = buff;
	if (root == NULL) return;
	struct ASTNode* line = root->right;
	dict_add(enviorment, "_importprefix", makeStr(prefix));
	while(line) {
		signature = buff;
		signature[0] = '\0';
		strcat(signature, prefix);
		if (*prefix != '\0') strcat(signature, "_");
		struct ASTNode* variablei = line->right;
		if (variablei == NULL) { line = line->down; continue; } //Empty lines.
		struct ASTNode* function = variablei->right;
		if (function == NULL) runtime_error("(Prerun) Interpreter crashed on function node.");
		strcat(signature, function->value.as_str);
		struct ASTNode* forarg = function->right;
		while(forarg && forarg->type != AST_LINE) {
			switch(forarg->type) {
				case AST_CALL:
					strcat(signature, "_");
					strcat(signature, forarg->value.as_str); break;
				case AST_VAR:
					strcat(signature, "v"); break;
				case AST_VAL:
					strcat(signature, sdt_code_litsignature(forarg));
					break;
			}
			forarg = forarg->right;
		};
		
		//Check correct types
		if (variablei->value.type != STR) runtime_error("(Prerun) Instruction variable token type mismatch!");
		if (function->value.type != STR) runtime_error("(Prerun) Instruction function token type mismatch!");
		
		if(function->type == AST_DECL) {
			Var t = dict_get(enviorment, "_importprefix");
			dict_add(enviorment, "_importprefix", makeStr(signature));
			sdt_decl(variablei->value.as_str, signature, function);
			sdt_prerun(function, signature);
			dict_add(enviorment, "_importprefix", t);
		}
		else if(function->type == AST_CALL && strcmp(function->value.as_str, "IMPORT") == 0) {
			struct ASTNode* arg0 = function->right;
			if (arg0 == NULL) { line = line->down; continue; }
			struct List* list = list_new();
			if (arg0->type == AST_VAL) list_add(list, 0, copyVar(arg0->value));
			if (arg0->type == AST_VAR) list_add(list, 0, copyVar(dict_get(enviorment, arg0->value.as_str)));
			
			sdt_call("", "IMPORTv", list, function);
			
			list_clear(list);
			free(list);
		}
		line = line->down;
	}
}

void sdt_run(struct ASTNode* root) {
	static char buff[1024];
	char* signature = buff;
	if (root == NULL) return;
	//printf("(run) ROOT: %s\n", root->value.as_str); fflush(stdout);
	struct List* arglist = list_new();
	struct ASTNode* line = root->right;
	while(line) {
		linenode = line;
		signature = buff;
		signature[0] = '\0';
		
		struct ASTNode* variablei = line->right;
		if (variablei == NULL) { line = line->down; continue; } //Empty lines.
		struct ASTNode* function = variablei->right;
		if (function == NULL) runtime_error("Interpreter crashed on function node.");
		strcat(signature, function->value.as_str);
		struct ASTNode* forarg = function->right;
		while(forarg && forarg->type != AST_LINE) {
			if (forarg->type == AST_VAL) list_add(arglist, arglist->size, copyVar(forarg->value));
			if (forarg->type == AST_VAR) list_add(arglist, arglist->size, copyVar(dict_get(enviorment, forarg->value.as_str)));
			if (forarg->type == AST_CALL) {
				strcat(signature, "_");
				strcat(signature, forarg->value.as_str);
			} else {
				strcat(signature, "v");
			}
			forarg = forarg->right;
		};
		
		//Check correct types
		if (variablei->value.type != STR) runtime_error("Instruction variable token type mismatch!");
		if (function->value.type != STR) runtime_error("Instruction function token type mismatch!");
		
		switch(function->type) {
			case AST_CALL:
				if (strcmp(function->value.as_str, "RETURN") == 0) {
					variable = variablei->value.as_str;
					p_return = dict_get(enviorment, "_declast").as_ast;
					break;
				}
				if (strcmp(function->value.as_str, "IMPORTv") == 0) { //Don't run imports!
					variable = variablei->value.as_str;
					break;
				}
				sdt_call(variablei->value.as_str, signature, arglist, function); break;
			case AST_DECL:
				//sdt_decl(variablei->value.as_str, function->value.as_str, function);
				break;
		}
		list_clear(arglist);
		line = line->down;
		
		if (p_return != NULL) break;
	}
	free(arglist);
}

void sdt_run_main() {
	struct List* arglist = list_new();
	sdt_call("", "MAIN", arglist, root);
	free(arglist);
}