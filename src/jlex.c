#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../include/jack.h"
#include "../include/jlex.h"

const char * const tokenTypeNames[] = { "keyword",
										"identifier",
										"operator",
										"string",
										"integer",
										"punctuator",
										"terminator"
									};

const char * const keywords[] = {	"boolean",
									"char",
									"class",
									"constructor",
									"do",
									"else",
									"false",
									"field",
									"function",
									"if",
									"int",
									"let",
									"method",
									"null",
									"return",
									"static",
									"true",
									"this",
									"var",
									"void",
									"while"
								};

const char * const operators = "+-*/&|~<>=";
const char * const punctuators = "({[]}),.;";

int lineNum = 1;
token peekedToken = { 0 };
long int peekOffset = 0;

static inline bool isoperator(int c)
{
	for(unsigned int i = 0; i < strlen(operators); i++)
		if(c == operators[i])
			return true;

	return false;
}

static inline bool ispunctuator(int c)
{
	for(unsigned int i = 0; i < strlen(punctuators); i++)
		if(c == punctuators[i])
			return true;

	return false;
}

static inline bool iskeyword(char * string)
{
	for(unsigned int i = 0; i < sizeof(keywords) / sizeof(char *); i++)
		if(!strcmp(keywords[i], string))
			return true;

	return false;
}

static int _getNextToken(token * nextToken)
{
	bool inSingleComment = false;
	bool inMultiComment = false;
	int c;

	/* Strip all whitespace and comments preceeding a token */

	do {
		c = fgetc(sourceFile);

		if(c == '/') {
			if((c = fgetc(sourceFile)) == '/') {
				inSingleComment = true;
			} else if(c == '*') {
				inMultiComment = true;
			} else {
				ungetc(c, sourceFile);
				c = '/';
			}
		} else if(c == '*' && inMultiComment) {
			if((c = fgetc(sourceFile)) == '/') {
				inMultiComment = false;
				c = fgetc(sourceFile);
			}
		}

		if(c == '\n') {
			lineNum++;
			if(inSingleComment) {
				inSingleComment = false;
			}
		}
	} while(isspace(c) || ((inSingleComment || inMultiComment) && c != EOF));

	nextToken->lineNum = lineNum;
	nextToken->character = c;

	/* Check if the lexeme is a single character */

	if(c == EOF) {
		nextToken->type = terminator;
		lineNum = 1;
		return EXEC_SUCCESS;
	} else if(isoperator(c)) {
		nextToken->type = operator;
		return EXEC_SUCCESS;
	} else if(ispunctuator(c)) {
		nextToken->type = punctuator;
		return EXEC_SUCCESS;
	}

	/* If it's not a single character then the lexeme must be multi-character and we will need additional storage */

	int pos = 0;
	char tempString[MAX_LEXEME_SIZE] = { '\0' };

	if(isdigit(c)) { 
		do {
			tempString[pos++] = c;
			c = fgetc(sourceFile);
		} while(isdigit(c) && pos < MAX_LEXEME_SIZE - 1);

		ungetc(c, sourceFile);

		if(!isoperator(c) && !ispunctuator(c) && !isspace(c)) {
			return LEX_ERROR;
		}

		nextToken->type = integer;
	} else if(c == '"') {
		do {
			tempString[pos++] = c;
			c = fgetc(sourceFile);

			if(c == '\n' || c == EOF) {
				return LEX_ERROR;
			}
		} while(c != '"' && pos < MAX_LEXEME_SIZE - 2);

		tempString[pos] = '"';
		nextToken->type = string;
	} else { /* If it's not a number or a string literal then the it must be either an identifier or a keyword */
		do {
			tempString[pos++] = c;
			c = fgetc(sourceFile);
		} while((isalpha(c) || isdigit(c) || c == '_') && pos < MAX_LEXEME_SIZE - 1);

		ungetc(c, sourceFile);
		nextToken->type = (iskeyword(tempString) ? keyword : identifier);
	}

	if(!(nextToken->string = malloc(strlen(tempString) + 1)))
		return MEM_ERROR;

	memcpy(nextToken->string, tempString, strlen(tempString) + 1);

	return EXEC_SUCCESS;
}

static int _peekNextToken(token * currToken)
{
	if(peekOffset) { 
		memcpy(currToken, &peekedToken, sizeof(token)); /* If the current token has been previously peeked then we don't need to go through the lexing process again */
		return EXEC_SUCCESS;
	}

	long int curPos = ftell(sourceFile); /* If it hasn't then we will need to find the current position in the file to move back to once we've extracted it */
	int lexerStatus = _getNextToken(currToken);

	peekOffset = ftell(sourceFile);

	memcpy(&peekedToken, currToken, sizeof(token));
	fseek(sourceFile, curPos, SEEK_SET); /* Move back to the begining of the token */

	return lexerStatus;
}

void getNextToken(token * currToken)
{
	if(peekOffset) {
		memcpy(currToken, &peekedToken, sizeof(token)); /* As above, if we've already extracted the token then just send it again */
		fseek(sourceFile, peekOffset, SEEK_SET); /* Since the file position will be before the current token we need to move it to the end of the token */

		peekOffset = 0;

		return;
	}

	if(_getNextToken(currToken) != EXEC_SUCCESS) {
		fprintf(stderr, "Error: Could not get next token!\n");
		exit(LEX_ERROR);
	}
}

void peekNextToken(token * currToken)
{
	if(_peekNextToken(currToken) != EXEC_SUCCESS) {
		fprintf(stderr, "Error: Could not get next token!\n");
		exit(LEX_ERROR);
	}
}