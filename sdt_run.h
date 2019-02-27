//Functions used to interpret (not compile), Xirirta code.

#ifndef XIRITA_SDT_RUN_H
#define XIRITA_SDT_RUN_H

#include "structs.h"

void  sdt_init();
void  sdt_run(struct ASTNode* root);
void  sdt_prerun(struct ASTNode* root, char* prefix);
void  sdt_run_main();
char* sdt_code_litsignature(const struct ASTNode* x);
#endif