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
extern expression * curExpression;
extern term * curTerm;

extern bool returned;

statement * parseStatement()
{
	token currToken;

	peekNextToken(&currToken);

	if(currToken.type != keyword)
		syntaxError("Statement or \'}\'", currToken);
	else if(!strcmp(currToken.string, "var"))
		return parseVarDeclarStatement();
	else if(!strcmp(currToken.string, "let"))
		return parseLetStatement();
	else if(!strcmp(currToken.string, "if"))
		return parseIfStatement();
	else if(!strcmp(currToken.string, "while"))
		return parseWhileStatement();
	else if(!strcmp(currToken.string, "do"))
		return parseDoStatement();
	else if(!strcmp(currToken.string, "return"))
		return parseReturnStatement();
	else
		syntaxError("Statement or \'}\'", currToken);

	return NULL;
}

statement * parseVarDeclarStatement()
{
	token currToken;
	variableSymbol * curVariable;

	getNextToken(&currToken);

	if(currentFunction->statements)
		semanticWarning("Variable declared after statements");
	
	syntaxOkay(currToken);
	getNextToken(&currToken);

	curVariable = newVariableSymbol();
	curVariable->lineNum = currToken.lineNum;
	curVariable->type = variable;
	
	setVariableTypeName(curVariable, currToken.string);
	syntaxOkay(currToken);
	getNextToken(&currToken);

	if(currToken.type != identifier)
		syntaxError("Identifier", currToken);

	setVariableName(curVariable, currToken.string);
	addVariableToFunction(currentFunction, curVariable);
	syntaxOkay(currToken);

	for(;;) {
		getNextToken(&currToken);

		if(currToken.type == punctuator) {
			if(currToken.character == ',') {
				syntaxOkay(currToken);
				getNextToken(&currToken);

				curVariable = newVariableSymbol();
				
				setVariableTypeName(curVariable, currentFunction->lastVariable->typeName);

				if(currToken.type != identifier)
					syntaxError("Identifier", currToken);

				setVariableName(curVariable, currToken.string);
				addVariableToFunction(currentFunction, curVariable);
				syntaxOkay(currToken);
			} else if(currToken.character == ';') {
				syntaxOkay(currToken);
				break;
			}
		} else {
			syntaxError("\',\' or \';\'", currToken);
		}
	}

	return NULL; // All variable declarations are not considered statements in the same sense as others and can be discarded
}

statement * parseLetStatement()
{
	token currToken;
	statement * curStatement;

	curStatement = newStatement(letStatement);
	
	getNextToken(&currToken);
	syntaxOkay(currToken);
	getNextToken(&currToken);

	curStatement->lineNum = currToken.lineNum;

	if(currToken.type != identifier)
		syntaxError("Identifier", currToken);

	if(!(curStatement->target = calloc(strlen(currToken.string) + 1, 1))) {
		fprintf(stderr, "Error: Cannot allocate memory for variable name!\n");
		exit(MEM_ERROR);
	}

	strncpy(curStatement->target, currToken.string, strlen(currToken.string));

	syntaxOkay(currToken);
	getNextToken(&currToken);

	if(currToken.type == punctuator || currToken.type == operator) {
		if(currToken.character == '=') {
			syntaxOkay(currToken);
		} else if(currToken.character == '[') {
			syntaxOkay(currToken);

			curStatement->indexExpression = parseExpression(); // Since expressions can exist inside other expressions I won't have a global expresion variable as that will get very messy very quickly
			
			getNextToken(&currToken);

			if(currToken.type == punctuator && currToken.character == ']') {
				syntaxOkay(currToken);
				getNextToken(&currToken);

				if(currToken.type == operator && currToken.character == '=') {
					syntaxOkay(currToken);
				} else {
					syntaxError("\'=\'", currToken);
				}
			} else {
				syntaxError("\']\'", currToken);
			}
		}
	} else {
		syntaxError("\'[\' or \'=\'", currToken);
	}

	curStatement->expression = parseExpression();

	getNextToken(&currToken);

	if(currToken.type != punctuator || currToken.character != ';')
		syntaxError("\';\'", currToken);

	syntaxOkay(currToken);

	return curStatement;	
}

statement * parseIfStatement()
{
	token currToken;
	statement * curStatement;

	curStatement = newStatement(ifStatement);

	getNextToken(&currToken);
	syntaxOkay(currToken);
	getNextToken(&currToken);

	curStatement->lineNum = currToken.lineNum;

	if(currToken.type != punctuator || currToken.character != '(')
		syntaxError("\'(\'", currToken);
	
	syntaxOkay(currToken);

	curStatement->ifCondition = parseExpression();
	
	getNextToken(&currToken);

	if(currToken.type != punctuator || currToken.character != ')')
		syntaxError("\')\'", currToken);

	syntaxOkay(currToken);
	getNextToken(&currToken);

	if(currToken.type != punctuator || currToken.character != '{')
		syntaxError("\'{\'", currToken);
	
	syntaxOkay(currToken);

	for(;;) {
		peekNextToken(&currToken);

		if(currToken.type != punctuator && currToken.character != '}') {
			if(!curStatement->ifStatements) {
				curStatement->ifStatements = curStatement->lastIfStatement = parseStatement();
			} else {
				curStatement->lastIfStatement->nextStatement = parseStatement();
				curStatement->lastIfStatement = curStatement->lastIfStatement->nextStatement;
			}
		} else {
			break;
		}
	}

	getNextToken(&currToken);

	if(currToken.type != punctuator || currToken.character != '}')
		syntaxError("\'}\'", currToken);

	syntaxOkay(currToken);
	peekNextToken(&currToken);

	if(currToken.type == keyword && !strcmp(currToken.string, "else")) {
		getNextToken(&currToken);
		syntaxOkay(currToken);
		getNextToken(&currToken);

		if(currToken.type != punctuator || currToken.character != '{')
			syntaxError("\'{\'", currToken);

		syntaxOkay(currToken);

		for(;;) {
			peekNextToken(&currToken);

			if(currToken.type != punctuator && currToken.character != '}') {
				if(!curStatement->elseStatements) {
					curStatement->elseStatements = curStatement->lastElseStatement = parseStatement();
				} else {
					curStatement->lastElseStatement->nextStatement = parseStatement();
					curStatement->lastElseStatement = curStatement->lastElseStatement->nextStatement;
				}
			} else {
				break;
			}
		}

		getNextToken(&currToken);

		if(currToken.type != punctuator || currToken.character != '}')
			syntaxError("\'}\'", currToken);
		
		syntaxOkay(currToken);
	}

	return curStatement;	
}

statement * parseWhileStatement()
{
	token currToken;
	statement * curStatement;

	curStatement = newStatement(whileStatement);

	getNextToken(&currToken);
	syntaxOkay(currToken);
	getNextToken(&currToken);

	curStatement->lineNum = currToken.lineNum;

	if(currToken.type != punctuator || currToken.character != '(')
		syntaxError("\'(\'", currToken);

	syntaxOkay(currToken);

	curStatement->whileCondition = parseExpression();

	getNextToken(&currToken);

	if(currToken.type != punctuator || currToken.character != ')')
		syntaxError("\')\'", currToken);

	syntaxOkay(currToken);
	getNextToken(&currToken);

	if(currToken.type != punctuator || currToken.character != '{')
		syntaxError("\'{\'", currToken);

	syntaxOkay(currToken);

	for(;;) {
		peekNextToken(&currToken);

		if(currToken.type != punctuator && currToken.character != '}') {
			if(!curStatement->whileStatements) {
				curStatement->whileStatements = curStatement->lastWhileStatement = parseStatement();
			} else {
				curStatement->lastWhileStatement->nextStatement = parseStatement();
				curStatement->lastWhileStatement = curStatement->lastWhileStatement->nextStatement;
			}
		} else {
			break;
		}
	}

	getNextToken(&currToken);

	if(currToken.type != punctuator || currToken.character != '}')
		syntaxError("\'}\'", currToken);
	
	syntaxOkay(currToken);

	return curStatement;	
}

statement * parseDoStatement()
{
	token currToken;
	statement * curStatement;

	curStatement = newStatement(doStatement);

	getNextToken(&currToken);
	syntaxOkay(currToken);

	curStatement->call = parseSubroutineCall();

	getNextToken(&currToken);

	curStatement->lineNum = currToken.lineNum;

	if(currToken.type != punctuator || currToken.character != ';')
		syntaxError("\';\'", currToken);
	
	syntaxOkay(currToken);

	return curStatement;
}

statement * parseReturnStatement()
{
	token currToken;
	statement * curStatement;

	getNextToken(&currToken);
	syntaxOkay(currToken);

	returned = true;
	curStatement = newStatement(returnStatement);

	peekNextToken(&currToken);

	if(currToken.type != punctuator && currToken.character != ';')
		curStatement->returnExpression = parseExpression();

	getNextToken(&currToken);

	curStatement->lineNum = currToken.lineNum;

	if(currToken.type != punctuator || currToken.character != ';')
		syntaxError("\';\'", currToken);
	
	syntaxOkay(currToken);

	return curStatement;	
}