#include <stdio.h>
#include <stdlib.h>

#include "../include/jack.h"
#include "../include/jlex.h"
#include "../include/jparse.h"
#include "../include/jsym.h"
#include "../include/jgen.h"

FILE * sourceFile;
extern classSymbolTable * classes;
extern int lineNum;

int main(int argc, char * argv[])
{
	if(argc > 1) {
		for(int i = 1; i < argc; i++) {
			printf("[+] Processing \"%s\"...\n", argv[i]);
			printf("[-] Opening file...");
			fflush(stdout);

			if(!(sourceFile = fopen(argv[i], "r"))) {
				fprintf(stderr, "Error: Could not open file \'%s\'!\n", argv[i]);
				return FILE_ERROR;
			}

			printf("Success!\n[-] Parsing...");
			fflush(stdout);

			/*printf("\n\nResults\n\nToken Name\tToken Type\tLine Number\n");*/
			
			parseClass(); /* Generates a parse tree of the current class */
			puts("Done!");
			fclose(sourceFile);
		}

		printf("[+] Finalising symbol table...");
		fflush(stdout);

		finaliseSymbolTables();

		printf("Done!\n[+] Generating code...");
		fflush(stdout);

		generateCode();

		puts("Done!");
		
		freeClasses();
	} else {
		fprintf(stderr, "Error: No input files given!\n\nUsage: ./%s [input files]\n", argv[0]);
		return FILE_ERROR;
	}

	puts("[+] All input files processed!\nTerminating...");
		
	return EXEC_SUCCESS;
}