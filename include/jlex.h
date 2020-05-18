#ifndef JLEX_H
#define JLEX_H

#include <stdio.h>	/* Required for FILE data type */

#define DEFAULT_LIST_SIZE 1024
#define MAX_LEXEME_SIZE 4096

typedef enum tokenTypes { keyword, identifier, operator, string, integer, punctuator, terminator, character } tokenName;

typedef struct token {
	union {
		char * string;
		int character;
	};
	tokenName type;
	int lineNum;
} token;

extern const char * const keywords[];
extern const char * const operators;
extern const char * const punctuators;
extern const char * const tokenTypeNames[];

void getNextToken(token * currToken);
void peekNextToken(token * currToken);

#endif