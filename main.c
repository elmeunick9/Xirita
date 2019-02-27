#include "utils.h"
#include "sdt_run.h"

extern void run_lexer();
extern void run_parser();
extern void run_program();

unsigned int script_fileno;

int main(int argc, const char* argv[]) {
	char buff[64];
	char* script = buff; strncpy(script, "main.x", 10);
	if (argc > 1) strncpy(script, argv[1], 60);
	
	FILE *in_file  = fopen(script, "r");
	if (in_file == NULL) {
		fprintf(stderr, "Can't open input file!\n");
		exit(1);
	}
	
	script_fileno = fileno(in_file);
	
	
	//run_lexer();
	run_parser();
	
	fclose(in_file);
	
	run_program();
}