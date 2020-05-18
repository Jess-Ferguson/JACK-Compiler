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

void parseClass()
{
	token currToken;

	getNextToken(&currToken);

	if(strcmp(currToken.string, "class"))
		syntaxError("Keyword \"class\"", currToken);

	syntaxOkay(currToken);
	getNextToken(&currToken);

	if(currToken.type != identifier)
		syntaxError("Identifier", currToken);

	newClassSymbolTable(currToken.string);
	syntaxOkay(currToken);
	getNextToken(&currToken);

	if(currToken.type != punctuator && currToken.character != '{')
		syntaxError("\'{\'", currToken);

	syntaxOkay(currToken);

	for(;;) {
		peekNextToken(&currToken);

		if(currToken.type == punctuator && currToken.character == '}') {
			getNextToken(&currToken);
			syntaxOkay(currToken);
			break;
		} else if(!strcmp(currToken.string, "field") || !strcmp(currToken.string, "static")) {
			parseClassVarDeclaration();
		} else if(!strcmp(currToken.string, "constructor") || !strcmp(currToken.string, "function") || !strcmp(currToken.string, "method")) {
			break;
		} else {
			syntaxError("Class variable or subroutine", currToken);
		}
	}

	for(;;) {
		peekNextToken(&currToken);

		if(currToken.type == punctuator && currToken.character == '}') {
			getNextToken(&currToken);
			syntaxOkay(currToken);
			break;
		} else if(!strcmp(currToken.string, "constructor") || !strcmp(currToken.string, "function") || !strcmp(currToken.string, "method")) {
			parseSubroutineDeclaration();
		} else {
			syntaxError("Class variable or subroutine", currToken);
		}
	}

	getNextToken(&currToken);

	if(currToken.type != terminator)
		syntaxError("Terminator", currToken);

	syntaxOkay(currToken);

	currentClass = NULL;

	return;
}

void parseClassVarDeclaration()
{
	token currToken;
	variableSymbol * curVariable;

	getNextToken(&currToken);

	curVariable = newVariableSymbol();

	if(!strcmp(currToken.string, "field"))
		curVariable->type = field;
	else if(!strcmp(currToken.string, "static"))
		curVariable->type = statik;
	else
		syntaxError("Keyword \"field\" or \"static\"", currToken);
	
	syntaxOkay(currToken);
	parseType();
	getNextToken(&currToken);
	setVariableTypeName(curVariable, currToken.string);
	syntaxOkay(currToken);
	getNextToken(&currToken);

	if(currToken.type != identifier)
		syntaxError("Identifier", currToken);

	setVariableName(curVariable, currToken.string);
	addVariableToClass(currentClass, curVariable);
	syntaxOkay(currToken);

	for(;;) {
		getNextToken(&currToken);

		if(currToken.type == punctuator) {
			if(currToken.character == ',') {
				syntaxOkay(currToken);

				curVariable = newVariableSymbol();
				curVariable->type = currentClass->lastVariable->type;

				setVariableTypeName(curVariable, currentClass->lastVariable->typeName);
				getNextToken(&currToken);

				if(currToken.type != identifier)
					syntaxError("Identifier", currToken);

				setVariableName(curVariable, currToken.string);
				addVariableToClass(currentClass, curVariable);
				syntaxOkay(currToken);
			} else if(currToken.character == ';') {
				syntaxOkay(currToken);
				break;
			}
		} else {
			syntaxError("\',\' or \';\'", currToken);
		}
	}

	return;
}