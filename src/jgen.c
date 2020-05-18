#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/jack.h"
#include "../include/jgen.h"
#include "../include/jsym.h"
#include "../include/jparse.h"

extern classList classes;
extern classSymbolTable * currentClass;
extern functionSymbolTable * currentFunction;
extern variableSymbol * currentVariable;
extern statement * curStatement;

FILE * curFile = NULL;
int labelID = 0;

void generateCode()
{
	currentClass = NULL;
	currentFunction = NULL;
	currentVariable = NULL;

	for(classSymbolTable * curClass = classes.firstClass; curClass; curClass = curClass->nextClass)
		processClass(curClass);

	return;
}

void processClass(classSymbolTable * curClass)
{
	char * filename;

	currentClass = curClass;
	labelID = 0;

	if(!isupper(curClass->name[0]))
		semanticWarning("Class name should start with capital letter");

	if(!(filename = calloc(strlen(curClass->name) + 4, 1))) {
		fprintf(stderr, "Error: Could not allocate memory for file name!\n");
		exit(MEM_ERROR);
	}

	snprintf(filename, strlen(curClass->name) + 4, "%s.vm", curClass->name);

	if(!(curFile = fopen(filename, "w"))) {
		fprintf(stderr, "Error: Could not open file \"%s\" for writing!\n", curClass->name);
		exit(FILE_ERROR);
	}

	for(functionSymbolTable * curFunction = curClass->functions; curFunction; curFunction = curFunction->nextFunction)
		processFunction(curFunction);

	free(filename);
	fclose(curFile);

	return;
}

void processFunction(functionSymbolTable * curFunction)
{
	currentFunction = curFunction;

	if(!islower(currentFunction->name[0]))
		semanticWarning("Function name should start with lowercase letter");

	fprintf(curFile, "function %s.%s %d\n", currentClass->name, curFunction->name, curFunction->variableCount);

	if(curFunction->type == constructor)
		fprintf(curFile, "push constant %d\ncall Memory.alloc 1\npop pointer 0\n", currentClass->fieldCount); /* TODO: Push the scope instead of the argument count */
	else if(curFunction->type == method)
		fprintf(curFile, "push argument 0\npop pointer 0\n");

	if(!processStatements(curFunction->statements) && strcmp(curFunction->typeName, "void"))
		semanticWarning("Non-void function not guaranteed to return a value");

	return;
}

bool processStatements(statement * currentStatement)
{
	if(!currentStatement)
		return false;

	curStatement = currentStatement;

	switch(currentStatement->type) {
		case ifStatement:
			if(processIfStatement(currentStatement)) {
				if(currentStatement->nextStatement) {
					semanticWarning("Unreachable code detected");
				}

				return true;
			}

			break;
		case letStatement:
			processLetStatement(currentStatement);
			break;
		case whileStatement:
			processWhileStatement(currentStatement);
			break; 
		case returnStatement:
			processReturnStatement(currentStatement);
			
			if(currentStatement->nextStatement)
				semanticWarning("Unreachable code detected");

			return true;
		case doStatement:
			processDoStatement(currentStatement);
			break;
		default:
			break;
	}

	return processStatements(currentStatement->nextStatement);
}

bool processIfStatement(statement * currentStatement)
{
	bool retval; /* Keep track of whether or not both the if/else blocks return a value for later semantic analysis */  
	int currentLabel = labelID++; /* Store an internal copy of the current label ID in case a nested loop or if/else block increments it */

	processExpression(currentStatement->ifCondition);
	fprintf(curFile, "if-goto IF_%d\n", currentLabel);

	retval = processStatements(currentStatement->elseStatements);

	fprintf(curFile, "goto ENDIF_%d\nlabel IF_%d\n", currentLabel, currentLabel);

	retval &= processStatements(currentStatement->ifStatements);

	fprintf(curFile, "label ENDIF_%d\n", currentLabel);

	return retval; 
}

void processLetStatement(statement * currentStatement)
{
	variableSymbol * curVariable;
	char * expressionType;

	if(!(curVariable = lookupFunctionVariable(currentFunction, currentStatement->target)) && !(curVariable = lookupClassVariable(currentClass, currentStatement->target)))
		semanticError("Undeclared identifier");

	/* Process the expression */

	expressionType = processExpression(currentStatement->expression);

	/* Push the variable being initialised to the stack */
	
	if(currentStatement->indexExpression) {
		fprintf(curFile, "push ");

		if(curVariable->type == statik)
			fprintf(curFile, "static ");
		else if(curVariable->type == field)
			fprintf(curFile, "this ");
		else if(curVariable->isArgument)
			fprintf(curFile, "argument ");
		else 
			fprintf(curFile, "local ");

		fprintf(curFile, "%d\n", curVariable->offset);

		if(strcmp(processExpression(currentStatement->indexExpression), "int"))
			semanticError("Array expression must be of integer type");

		fprintf(curFile, "add\npop pointer 1\npop that 0\n");
	} else {
		fprintf(curFile, "pop ");

		if(curVariable->type == statik)
			fprintf(curFile, "static ");
		else if(curVariable->type == field)
			fprintf(curFile, "this ");
		else if(curVariable->isArgument)
			fprintf(curFile, "argument ");
		else 
			fprintf(curFile, "local ");

		fprintf(curFile, "%d\n", curVariable->offset);

		if(strcmp(expressionType, curVariable->typeName))
			semanticWarning("Expression type does not match variable type");
	}

	curVariable->initialised = true;
	
	return;
}

void processWhileStatement(statement * currentStatement)
{
	int currentLabel = labelID++; /* Store an internal copy of the current label ID in case a nested loop or if/else block increments it */

	fprintf(curFile, "label WHILE_%d\n", currentLabel);
	processExpression(currentStatement->whileCondition);
	fprintf(curFile, "not\nif-goto END_WHILE_%d\n", currentLabel);
	processStatements(currentStatement->whileStatements);
	fprintf(curFile, "goto WHILE_%d\nlabel END_WHILE_%d\n", currentLabel, currentLabel);

	return;
}

void processReturnStatement(statement * currentStatement)
{
	if(!strcmp(currentFunction->typeName, "void")) {
		fprintf(curFile, "push constant 0\n");
	} else {
		if(strcmp(processExpression(currentStatement->returnExpression), currentFunction->typeName)) {
			semanticWarning("Type of returned expression does not match the type of the function");
		}
	}

	fprintf(curFile, "return\n");

	return;
}

void processDoStatement(statement * currentStatement)
{
	processFunctionCall(currentStatement->call);
	fprintf(curFile, "pop temp 0\n");

	return;
}

char * processExpression(expression * currentExpression)
{
	char * expressionType;

	if(!currentExpression)
		return "void";

	expressionType = processTerm(currentExpression->terms[0]);

	for(unsigned int i = 1; i < currentExpression->termCount; i++)
		if(strcmp(expressionType, processTerm(currentExpression->terms[i])))
			semanticWarning("Term in expression has invalid type");

	for(unsigned int j = 0; j < currentExpression->operatorCount; j++)
		processOperator(currentExpression->operators[j]);
		
	return expressionType;
}

void processOperator(char operator)
{
	switch(operator) {
		case '+':
			fprintf(curFile, "add\n");
			break;
		case '-':
			fprintf(curFile, "sub\n");
			break;
		case '*':
			fprintf(curFile, "call Math.multiply 2\n");
			break;
		case '/':
			fprintf(curFile, "call Math.divide 2\n");
			break;
		case '&':
			fprintf(curFile, "and\n");
			break;
		case '|':
			fprintf(curFile, "or\n");
			break;
		case '<':
			fprintf(curFile, "lt\n");
			break;
		case '>':
			fprintf(curFile, "gt\n");
			break;
		case '=':
			fprintf(curFile, "eq\n");
			break;
		default:
			break;
	}

	return;
}

char * processTerm(term * curTerm)
{
	char * termType;
	variableSymbol * curVariable;

	if(curTerm->type == constant) {
		if(curTerm->constantType == integerType) {
			fprintf(curFile, "push constant %s\n", curTerm->constantTerm);

			return "int";
		} else if(curTerm->constantType == stringType) {
			fprintf(curFile, "push constant %zu\ncall String.new 1\n", strlen(curTerm->constantTerm));

			for(unsigned int i = 1; curTerm->constantTerm[i + 1]; i++)
				fprintf(curFile, "push constant %d\ncall String.appendChar 2\n", curTerm->constantTerm[i]);

			return "String";
		} else {
			if(!strcmp(curTerm->constantTerm, "null")) {
				fprintf(curFile, "push constant 0\n");

				return "int";
			} else if(!strcmp(curTerm->constantTerm, "false")) {
				fprintf(curFile, "push constant 0\n");

				return "boolean";
			} else if(!strcmp(curTerm->constantTerm, "true")) {
				fprintf(curFile, "push constant 1\nneg\n");

				return "boolean";
			} else if(!strcmp(curTerm->constantTerm, "this")) {
				fprintf(curFile, "push pointer 0\n");

				return currentClass->name;
			} else {
				semanticError("Unknown constant type");
			}
		}
	} else if(curTerm->type == expr) {
		return processExpression(curTerm->expr);
	} else if(curTerm->type == unaryTerm) {
		termType = processTerm(curTerm->term);

		if(strcmp(termType, "int") && strcmp(termType, "boolean"))
			semanticWarning("Unary term is not a boolean or integer type");

		if(curTerm->operator == '-')
			fprintf(curFile, "neg\n");
		else if(curTerm->operator == '~')
			fprintf(curFile, "not\n");

		return termType;
	} else if(curTerm->type == reference) {
		if(!(curVariable = lookupFunctionVariable(currentFunction, curTerm->variableName)) && !(curVariable = lookupClassVariable(currentClass, curTerm->variableName)))
			semanticError("Undeclared identifier");

		if(!curVariable->initialised)
			semanticWarning("Use of variable before initialisation");

		fprintf(curFile, "push ");

		if(curVariable->type == statik)
			fprintf(curFile, "static ");
		else if(curVariable->type == field)
			fprintf(curFile, "this ");
		else if(curVariable->isArgument)
			fprintf(curFile, "argument ");
		else 
			fprintf(curFile, "local ");

		fprintf(curFile, "%d\n", curVariable->offset);

		return curVariable->typeName;
	} else if(curTerm->type == arrayReference) {
		if(!(curVariable = lookupFunctionVariable(currentFunction, curTerm->variableName)) && !(curVariable = lookupClassVariable(currentClass, curTerm->variableName)))
			semanticError("Undeclared identifier");

		if(strcmp(curVariable->typeName, "Array"))
			semanticWarning("Attempt to dereference non-array variable as an array");

		fprintf(curFile, "push ");

		if(curVariable->type == statik)
			fprintf(curFile, "static ");
		else if(curVariable->type == field)
			fprintf(curFile, "this ");
		else if(curVariable->isArgument)
			fprintf(curFile, "argument ");
		else 
			fprintf(curFile, "local ");

		fprintf(curFile, "%d\n", curVariable->offset);

		termType = processExpression(curTerm->indexExpression);

		if(strcmp(termType, "int"))
			semanticWarning("Array index is not of integer type");

		fprintf(curFile, "add\npop pointer 1\npush that 0\n");

		return "int";
	} else if(curTerm->type == funcCall) {
		return processFunctionCall(curTerm->call);
	}

	return NULL;
}

char * processFunctionCall(functionCall * call)
{
	bool isClassCall = false;
	int myOffset = 0;
	int dotIndex = -1;
	classSymbolTable * curClass;
	functionSymbolTable * curFunction;
	variableSymbol * curVariable;

	for(unsigned int i = 0; i < strlen(call->actionName); i++) {
		if(call->actionName[i] == '.') {
			isClassCall = true;
			dotIndex = i;
		}
	}
	
	if(isClassCall) {
		call->actionName[dotIndex] = '\0';

		/* Determine if the reference is an object or a class */

		for(curClass = classes.firstClass; curClass; curClass = curClass->nextClass) {
			if(!strcmp(curClass->name, call->actionName)) {
				break;
			}
		}

		if(curClass) { /* If it is a function call then we check that the function exists within that class */
			if(!(curFunction = lookupClassFunction(curClass, call->actionName + dotIndex + 1))) {
				semanticError("Function does not exist");
			}
		} else { /* If not then it must be an object invoking a method, so we first check that the object exists within scope */
			if(!(curVariable = lookupFunctionVariable(currentFunction, call->actionName)) && !(curVariable = lookupClassVariable(currentClass, call->actionName)))
				semanticError("Undeclared identifier");

			/* Check that the object is of a class type that actuall exists and that that class contains a method of the same name */

			for(curClass = classes.firstClass; curClass; curClass = curClass->nextClass)
				if(!strcmp(curClass->name, curVariable->typeName))
					break;

			if(!curClass)
				semanticError("Variable is of unknown type");

			/* Check that the type has a method of the same name */

			if(!(curFunction = lookupClassFunction(curClass, call->actionName + dotIndex + 1)))
				semanticError("Function does not exist");

			fprintf(curFile, "push ");

			if(curVariable->type == statik)
				fprintf(curFile, "static ");
			else if(curVariable->type == field)
				fprintf(curFile, "this ");
			else if(curVariable->isArgument)
				fprintf(curFile, "argument ");
			else 
				fprintf(curFile, "local ");

			fprintf(curFile, "%d\n", curVariable->offset);
		}

		curVariable = curFunction->arguments;

		for(unsigned int i = 0; i < call->expressionCount && curVariable; i++, curVariable = curVariable->nextVariable)
			if(strcmp(processExpression(call->expressionList[i]), curVariable->typeName))
				semanticWarning("Expression type does not match parameter type");

		fprintf(curFile, "call %s.%s %d\n", curClass->name, call->actionName + dotIndex + 1, curFunction->argumentCount + myOffset);
	} else {
		if(!(curFunction = lookupClassFunction(currentClass, call->actionName)))
			semanticError("Function does not exist");

		if(curFunction->type != method)
			myOffset = -2;

		fprintf(curFile, "push pointer 0\n");

		curVariable = curFunction->arguments;

		for(unsigned int i = 0; i < call->expressionCount; i++, curVariable = curVariable->nextVariable)
			if(strcmp(processExpression(call->expressionList[i]), curVariable->typeName))
				semanticWarning("Expression type does not match parameter type");

		fprintf(curFile, "call %s.%s %d\n", currentClass->name, call->actionName, curFunction->argumentCount + myOffset);
	}
	
	return curFunction->typeName;
}