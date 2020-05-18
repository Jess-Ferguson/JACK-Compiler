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

extern FILE * sourceFile;

void syntaxError(char * expected, token currToken)
{
	if(currToken.type == keyword || currToken.type == integer || currToken.type == identifier || currToken.type == string)
		fprintf(stderr, "\nSyntax error: %s expected! Got \"%s\" instead (line %d)\n", expected, currToken.string, currToken.lineNum);
	else
		fprintf(stderr, "\nSyntax error: %s expected! Got \"%c\" instead (line %d)\n", expected, currToken.character, currToken.lineNum);

	freeClasses();
	fclose(sourceFile);
	exit(PARSE_ERROR);
}

/*void syntaxOkay(token currToken)
{
	if(	currToken.type == integer || currToken.type == keyword || currToken.type == identifier || currToken.type == string || currToken.type == character) {
		printf("%s", currToken.string);

		if(strlen(currToken.string) < TAB_WIDTH)
			putchar('\t');
			
		free(currToken.string);
	} else {
		printf("%c\t", currToken.character);
	}

	printf("\t%s\t", tokenTypeNames[currToken.type]);

	if(strlen(tokenTypeNames[currToken.type]) < TAB_WIDTH)
		putchar('\t');

	printf("%d\n",currToken.lineNum);

	fflush(stdout);

	return;
}*/

void syntaxOkay(token currToken)
{
	if(currToken.type == integer || currToken.type == keyword || currToken.type == identifier || currToken.type == string || currToken.type == character)
		free(currToken.string);

	return;
}

/* Statement functions */

statement * newStatement(statementType newStatementType)
{
	statement * newStatement;

	if(!(newStatement = calloc(sizeof(statement), 1))) {
		fprintf(stderr, "Error: Could not allocate memory for new statement!\n");
		exit(MEM_ERROR);
	}

	newStatement->type = newStatementType;

	return newStatement;
}

/* Expression functions */

expression * newExpression()
{
	expression * newExpr;

	if(!(newExpr = calloc(sizeof(expression), 1))) {
		fprintf(stderr, "Error: Could not allocate memory for new expression!\n");
		exit(MEM_ERROR);
	}

	return newExpr;
}

void addOperator(expression * curExpression, char operator)
{
	if(!(curExpression->operators = realloc(curExpression->operators, curExpression->operatorCount + 1))) {
		fprintf(stderr, "Error: Could not allocate memory for operator list!\n");
		exit(MEM_ERROR);
	}

	curExpression->operators[curExpression->operatorCount++] = operator;
}

void addTerm(expression * curExpression, term * curTerm)
{
	if(!(curExpression->terms = realloc(curExpression->terms, (curExpression->termCount + 1) * sizeof(term *)))) {
		fprintf(stderr, "Error: Could not allocate memory for term list!\n");
		exit(MEM_ERROR);
	}

	curExpression->terms[curExpression->termCount++] = curTerm;
}

/* Term functions */

term * newTerm()
{
	term * newTerm;

	if(!(newTerm = calloc(sizeof(term), 1))) {
		fprintf(stderr, "Error: Could not allocate memory for new term!\n");
		exit(MEM_ERROR);
	}

	return newTerm;
}

void addConst(term * curTerm, char * constant)
{
	if(!(curTerm->constantTerm = calloc(strlen(constant) + 1, 1))) {
		fprintf(stderr, "Error: Could not allocate memory for constant term!\n");
		exit(MEM_ERROR);
	}

	strncpy(curTerm->constantTerm, constant, strlen(constant));
}

/* Cleanup functions */

void freeStatement(statement * currentStatement)
{
	if(!currentStatement)
		return;

	switch(currentStatement->type) {
		case ifStatement:
			freeExpression(currentStatement->ifCondition);
			freeStatement(currentStatement->ifStatements);
			freeStatement(currentStatement->elseStatements);
			break;
		case letStatement:
			free(currentStatement->target);
			freeExpression(currentStatement->indexExpression);
			freeExpression(currentStatement->expression);
			break;
		case whileStatement:
			freeExpression(currentStatement->whileCondition);
			freeStatement(currentStatement->whileStatements);
			break; 
		case returnStatement:
			freeExpression(currentStatement->returnExpression);
			break;
		case doStatement:
			free(currentStatement->call->actionName);

			while(currentStatement->call->expressionCount)
				freeExpression(currentStatement->call->expressionList[--currentStatement->call->expressionCount]);

			free(currentStatement->call->expressionList);

			free(currentStatement->call);
			break;
		case varStatement:
			break;
		default:
			break;
	}

	freeStatement(currentStatement->nextStatement);
	free(currentStatement);
}

void freeExpression(expression * currentExpression)
{
	if(!currentExpression)
		return;

	while(currentExpression->termCount)
		freeTerm(currentExpression->terms[--currentExpression->termCount]);

	free(currentExpression->terms);
	free(currentExpression->operators);
	free(currentExpression);
}

void freeTerm(term * currentTerm)
{
	if(!currentTerm)
		return;

	switch(currentTerm->type) {
		case constant:
			free(currentTerm->constantTerm);
			break;
		case reference:
			free(currentTerm->variableName);
			break;
		case unaryTerm:
			freeTerm(currentTerm->term);
			break;
		case expr:
			freeExpression(currentTerm->expr);
			break;
		case arrayReference:
			free(currentTerm->arrayName);
			freeExpression(currentTerm->indexExpression);
			break;
		case funcCall:
			free(currentTerm->call->actionName);

			while(currentTerm->call->expressionCount)
				freeExpression(currentTerm->call->expressionList[--currentTerm->call->expressionCount]);

			free(currentTerm->call->expressionList);
			free(currentTerm->call);
			break;
		default:
			break;
	}

	free(currentTerm);
}
