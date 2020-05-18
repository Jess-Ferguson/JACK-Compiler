#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/jack.h"
#include "../include/jparse.h"
#include "../include/jlex.h"
#include "../include/jsym.h"

void parseType()
{
	token currToken;

	peekNextToken(&currToken);

	if(	currToken.type != identifier && strcmp(currToken.string, "int") && strcmp(currToken.string, "char") && strcmp(currToken.string, "boolean"))
		syntaxError("Identifier or variable type", currToken);

	return;
}

term * parseTerm()
{
	token currToken;
	term * curTerm;

	curTerm = newTerm();

	getNextToken(&currToken);

	if(currToken.type == integer) {
		curTerm->type = constant;
		curTerm->constantType = integerType;

		addConst(curTerm, currToken.string);
		syntaxOkay(currToken);

		return curTerm;
	} else if(currToken.type == string) {
		curTerm->type = constant;
		curTerm->constantType = stringType;

		addConst(curTerm, currToken.string);
		syntaxOkay(currToken);

		return curTerm;
	} else if(currToken.type == keyword) {
		if(strcmp(currToken.string, "true") && strcmp(currToken.string, "false") && strcmp(currToken.string, "null") && strcmp(currToken.string, "this"))
			syntaxError("Keyword \"true\", \"false\", \"null\", or \"this\"", currToken);

		curTerm->type = constant;
		curTerm->constantType = keywordType;

		addConst(curTerm, currToken.string);
		syntaxOkay(currToken);

		return curTerm;
	} else if(currToken.type == identifier) {
		char * name;

		if(!(name = calloc(strlen(currToken.string) + 1, 1))) {
			fprintf(stderr, "Error: Cannot allocate memory for identifier!\n");
			exit(MEM_ERROR);
		}

		strncpy(name, currToken.string, strlen(currToken.string));
		syntaxOkay(currToken);
		peekNextToken(&currToken);

		if(currToken.type == punctuator && currToken.character == '[') {
			curTerm->type = arrayReference;
			curTerm->arrayName = name;

			getNextToken(&currToken);
			syntaxOkay(currToken);

			curTerm->indexExpression = parseExpression();
				
			getNextToken(&currToken);

			if(currToken.type != punctuator || currToken.character != ']')
				syntaxError("\']\'", currToken);

			syntaxOkay(currToken);

			return curTerm;
		}

		if(currToken.type == punctuator && currToken.character == '.') {
			curTerm->type = funcCall;

			getNextToken(&currToken);
			syntaxOkay(currToken);
			getNextToken(&currToken);

			if(currToken.type != identifier)
				syntaxError("Identifier", currToken);

			if(!(curTerm->call = calloc(sizeof(functionCall), 1))) {
				fprintf(stderr, "Error: Could not allocate memory for subroutine call!\n");
				exit(MEM_ERROR);
			}

			if(!(name = realloc(name, strlen(name) + strlen(currToken.string) + 2))) {
				fprintf(stderr, "Error: Could not allocate memory for subroutine name!\n");
				exit(MEM_ERROR);
			}

			snprintf(name + strlen(name), strlen(currToken.string) + 2, ".%s", currToken.string);
		
			curTerm->call->actionName = name;

			syntaxOkay(currToken);
			getNextToken(&currToken);

			if(currToken.type == punctuator && currToken.character == '(') {
				syntaxOkay(currToken);
				parseExpressionList(curTerm->call);
				getNextToken(&currToken);

				if(currToken.type != punctuator || currToken.character != ')')
					syntaxError("\')\'", currToken);

				syntaxOkay(currToken);
			} else {
				syntaxError("\'(\'", currToken);
			}
		} else if(currToken.character == '(') {
			if(!(curTerm->call = calloc(sizeof(functionCall), 1))) {
				fprintf(stderr, "Error: Could not allocate memory for subroutine call!\n");
				exit(MEM_ERROR);
			}

			curTerm->type = funcCall;
			curTerm->call->actionName = name;

			getNextToken(&currToken);
			syntaxOkay(currToken);
			parseExpressionList(curTerm->call);
			getNextToken(&currToken);

			if(currToken.type != punctuator || currToken.character != ')')
				syntaxError("\')\'", currToken);

			syntaxOkay(currToken);
		} else {
			curTerm->type = reference;
			curTerm->variableName = name;
		}

		return curTerm;
	} else if(currToken.type == punctuator && currToken.character == '(') {
		syntaxOkay(currToken);

		curTerm->type = expr;
		curTerm->expr = parseExpression();

		getNextToken(&currToken);

		if(currToken.type != punctuator || currToken.character != ')')
			syntaxError("\')\'", currToken);

		syntaxOkay(currToken);

		return curTerm;
	} else if(currToken.type == operator && (currToken.character == '~' || currToken.character == '-')) {
		curTerm->type = unaryTerm;
		curTerm->operator = currToken.character;

		syntaxOkay(currToken);

		curTerm->term = parseTerm();

		return curTerm;
	} else {
		syntaxError("String, integer, identifier, \"true\", \"false\", \"null\", \"this\", or \'(\'", currToken);
	}

	return curTerm;
}

expression * parseExpression()
{
	token currToken;
	expression * newExpr;
	term * newTerm;

	if(!(newTerm = parseTerm()))
		return NULL;

	newExpr = newExpression();
	
	addTerm(newExpr, newTerm);

	for(;;) {
		peekNextToken(&currToken);

		if(currToken.type == operator) {
			getNextToken(&currToken);
			addOperator(newExpr, currToken.character);
			syntaxOkay(currToken);

			if(!(newTerm = parseTerm()))
				syntaxError("Term expected after operator", currToken);

			addTerm(newExpr, newTerm);
		} else {
			break;
		}
	}

	return newExpr;
}


void parseExpressionList(functionCall * call)
{
	token currToken;

	peekNextToken(&currToken);

	if(currToken.type == punctuator && currToken.character == ')')
		return;

	if(!(call->expressionList = calloc(1, sizeof(expression *)))) {
		fprintf(stderr, "Error: Could not allocate memory for expression list!\n");
		exit(MEM_ERROR);
	}

	call->expressionList[0] = parseExpression();

	for(call->expressionCount = 1;; call->expressionCount++) {
		peekNextToken(&currToken);

		if(currToken.type == punctuator && currToken.character == ',') {
			getNextToken(&currToken);
			syntaxOkay(currToken);

			if(!(call->expressionList = realloc(call->expressionList, (call->expressionCount + 1) * sizeof(expression *)))) {
				fprintf(stderr, "Error: Could not allocate memory for expression list!\n");
				exit(MEM_ERROR);
			}

			call->expressionList[call->expressionCount] = parseExpression();
		} else if(currToken.type == punctuator && currToken.character == ')') {
			break;
		} else {
			syntaxError("\',\' or \')\'", currToken);
		}
	}

	return;
}