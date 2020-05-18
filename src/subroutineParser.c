#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/jack.h"
#include "../include/jparse.h"
#include "../include/jlex.h"
#include "../include/jsym.h"

extern classList classes;
extern classSymbolTable * currentClass;
extern functionSymbolTable * currentFunction;
extern variableSymbol * currentVariable;

bool returned;

void parseSubroutineDeclaration()
{
	token currToken;
	functionSymbolTable * curFunction;

	getNextToken(&currToken);

	curFunction = newFunctionSymbolTable();
	curFunction->lineNum = currToken.lineNum;

	if(!strcmp(currToken.string, "constructor")) {
		curFunction->type = constructor;
	} else if(!strcmp(currToken.string, "function")) {
		curFunction->type = func;
	} else if(!strcmp(currToken.string, "method")) {
		curFunction->type = method;
		curFunction->argumentCount = 1;
	} else {
		syntaxError("Keyword \"constructor\", \"function\", or \"method\"", currToken);
	}

	syntaxOkay(currToken);
	peekNextToken(&currToken);

	if(strcmp(currToken.string, "void"))
		parseType();

	getNextToken(&currToken);
	setFunctionTypeName(curFunction, currToken.string);
	syntaxOkay(currToken);
	getNextToken(&currToken);

	if(currToken.type != identifier)
		syntaxError("Identifier", currToken);

	setFunctionName(curFunction, currToken.string);
	addFunctionToClass(currentClass, curFunction);
	syntaxOkay(currToken);
	getNextToken(&currToken);

	if(currToken.type != punctuator && currToken.character != '(')
		syntaxError("\'(\'", currToken);

	syntaxOkay(currToken);
	parseParamList();
	getNextToken(&currToken);

	if(currToken.type != punctuator && currToken.character != ')')
		syntaxError("\')\'", currToken);

	syntaxOkay(currToken);
	parseSubroutineBody();

	currentFunction = NULL;

	return;
}

void parseSubroutineBody()
{
	token currToken;

	getNextToken(&currToken);

	if(currToken.type != punctuator || currToken.character != '{')
		syntaxError("\'{\'", currToken);
	
	syntaxOkay(currToken);

	for(;;) {
		peekNextToken(&currToken);

		if(currToken.type == punctuator && currToken.character == '}') {
			getNextToken(&currToken);
			syntaxOkay(currToken);

			break;
		} else {
			addStatementToFunction(currentFunction, parseStatement());
		}
	}

	return;
}

void parseParamList()
{
	token currToken;
	variableSymbol * curVariable;

	peekNextToken(&currToken);

	if(currToken.type == punctuator && currToken.character == ')')
		return;

	curVariable = newVariableSymbol();
	curVariable->lineNum = currToken.lineNum;

	parseType();
	getNextToken(&currToken);
	setVariableTypeName(curVariable, currToken.string);
	syntaxOkay(currToken);
	getNextToken(&currToken);

	if(currToken.type != identifier)
		syntaxError("Identifier", currToken);

	setVariableName(curVariable, currToken.string);
	addArgumentToFunction(currentFunction, curVariable);
	syntaxOkay(currToken);

	for(;;) {
		peekNextToken(&currToken);

		if(currToken.type == punctuator && currToken.character == ')') {
			return;
		} else if(currToken.type == punctuator && currToken.character == ',') {
			getNextToken(&currToken);
			syntaxOkay(currToken);

			curVariable = newVariableSymbol();

			parseType();
			getNextToken(&currToken);
			setVariableTypeName(curVariable, currToken.string);
			syntaxOkay(currToken);
			getNextToken(&currToken);

			if(currToken.type != identifier)
				syntaxError("Identifier", currToken);

			setVariableName(curVariable, currToken.string);
			addArgumentToFunction(currentFunction, curVariable);
			syntaxOkay(currToken);
		} else {
			syntaxError("\')\' or \',\'", currToken);
		}
	}
}

functionCall * parseSubroutineCall()
{
	token currToken;
	functionCall * call;

	getNextToken(&currToken);

	if(currToken.type != identifier)
		syntaxError("Identifier", currToken);

	if(!(call = calloc(1, sizeof(functionCall)))) {
		fprintf(stderr, "Error: Could not allocate memory for subroutine call!\n");
		exit(MEM_ERROR);
	}

	if(!(call->actionName = calloc(strlen(currToken.string) + 1, 1))) {
		fprintf(stderr, "Error: Could not allocate memory for subroutine name!\n");
		exit(MEM_ERROR);
	}

	strncpy(call->actionName, currToken.string, strlen(currToken.string));
	syntaxOkay(currToken);
	getNextToken(&currToken);

	if(currToken.type == punctuator) {
		if(currToken.character == '.') {
			syntaxOkay(currToken);
			getNextToken(&currToken);

			if(currToken.type != identifier)
				syntaxError("Identifier", currToken);

			if(!(call->actionName = realloc(call->actionName, strlen(call->actionName) + strlen(currToken.string) + 2))) {
				fprintf(stderr, "Error: Could not allocate memory for subroutine name!\n");
				exit(MEM_ERROR);
			}

			snprintf(call->actionName + strlen(call->actionName), strlen(currToken.string) + 2, ".%s", currToken.string);
			syntaxOkay(currToken);
			getNextToken(&currToken);
		}

		if(currToken.type == punctuator && currToken.character == '(') {
			syntaxOkay(currToken);
			parseExpressionList(call);
			getNextToken(&currToken);

			if(currToken.type != punctuator || currToken.character != ')')
				syntaxError("\')\'", currToken);

			syntaxOkay(currToken);
		} else {
			syntaxError("\'(\'", currToken);
		}
	} else {
		syntaxError("\'.\' or \'(\'", currToken);
	}

	return call;
}