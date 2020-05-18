#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/jack.h"
#include "../include/jsym.h"
#include "../include/jlex.h"

classList classes = { 0 };
classSymbolTable * currentClass;
functionSymbolTable * currentFunction;
variableSymbol * currentVariable;
statement * curStatement;

extern FILE * curFile;

/* Functions for reporting semantic errors during the finalisation stage */

void semanticWarning(const char * warning)
{
	if(!curStatement)
		fprintf(stderr, "Semantic warning in class \"%s\": %s!\n", currentClass->name, warning);
	else
		fprintf(stderr, "Semantic warning in class \"%s\": %s! (line %d)\n", currentClass->name, warning, curStatement->lineNum);
}

void semanticError(const char * error)
{
	if(!curStatement)
		fprintf(stderr, "\nSemantic Error in class \"%s\": %s!\n", currentClass->name, error);
	else
		fprintf(stderr, "\nSemantic Error in class \"%s\": %s! (line %d)\n", currentClass->name, error, curStatement->lineNum);

	if(curFile)
		fclose(curFile);

	freeClasses();
	exit(SEMANTIC_ERROR);
}

void finalisationError(const char * error, int lineNum)
{
	if(!currentClass)
		fprintf(stderr, "\nSemantic error in class \"%s\": %s! (line %d)\n", currentClass->name, error, lineNum);
	else
		fprintf(stderr, "\nSemantic error in class: %s! (line %d)\n", error, lineNum);

	freeClasses();
	exit(SEMANTIC_ERROR);
}

/* Symbol and symbol table initialisation functions */

classSymbolTable * newClassSymbolTable(char * name)
{
	classSymbolTable * curClass;

	if(!(curClass = calloc(1, sizeof(classSymbolTable)))) {
		fprintf(stderr, "Error: Could not allocate memory for class symbol table!\n");
		exit(MEM_ERROR);
	}

	if(!(curClass->name = calloc(strlen(name) + 1, 1))) {
		fprintf(stderr, "Error: Could not allocate memory for class name!\n");
		exit(MEM_ERROR);
	}

	strncpy(curClass->name, name, strlen(name));

	if(!classes.lastClass) {
		classes.firstClass = classes.lastClass = curClass;
	} else {
		classes.lastClass->nextClass = curClass;
		classes.lastClass = curClass;
	}

	currentClass = curClass;

	return curClass;
}

functionSymbolTable * newFunctionSymbolTable()
{
	functionSymbolTable * curFunction;

	if(!(curFunction = calloc(1, sizeof(functionSymbolTable)))) {
		fprintf(stderr, "Error: Could not allocate memory for function symbol table!\n");
		exit(MEM_ERROR);
	}

	currentFunction = curFunction;

	return curFunction;
}

variableSymbol * newVariableSymbol()
{
	variableSymbol * curVariable;

	if(!(curVariable = calloc(1, sizeof(variableSymbol)))) {
		fprintf(stderr, "Error: Could not allocate memory for variable symbol!\n");
		exit(MEM_ERROR);
	}

	return curVariable;
}

/* Functions for adding symbols to a class */

void addFunctionToClass(classSymbolTable * curClass, functionSymbolTable * curFunction)
{
	curClass->functionCount++;

	if(!curClass->functions) {
		curClass->functions = curClass->lastFunction = curFunction;
	} else {
		curClass->lastFunction->nextFunction = curFunction;
		curClass->lastFunction = curFunction;
	}

	return;
}

void addVariableToClass(classSymbolTable * curClass, variableSymbol * curVariable)
{
	if(curVariable->type == statik)
		curVariable->offset = curClass->staticCount++;
	else
		curVariable->offset = curClass->fieldCount++;

	if(!curClass->variables) {
		curClass->variables = curClass->lastVariable = curVariable;
	} else {
		curClass->lastVariable->nextVariable = curVariable;
		curClass->lastVariable = curVariable;
	}

	return;
}

/* Functions for adding variables to functions */

void addArgumentToFunction(functionSymbolTable * curFunction, variableSymbol * curVariable)
{
	curVariable->offset = curFunction->argumentCount++;

	if(!curFunction->arguments) {
		curFunction->arguments = curFunction->lastArgument = curVariable;
	} else {
		curFunction->lastArgument->nextVariable = curVariable;
		curFunction->lastArgument = curVariable;
	}

	return;
}

void addVariableToFunction(functionSymbolTable * curFunction, variableSymbol * curVariable)
{
	curVariable->offset = curFunction->variableCount++;

	if(!curFunction->variables) {
		curFunction->variables = curFunction->lastVariable = curVariable;
	} else {
		curFunction->lastVariable->nextVariable = curVariable;
		curFunction->lastVariable = curVariable;
	}

	return;
}

/* Functions for setting properties of functions */

void setFunctionName(functionSymbolTable * curFunction, char * name)
{
	if(!(curFunction->name = calloc(strlen(name) + 1, 1))) {
		fprintf(stderr, "Error: Could not allocate memory for function name!\n");
		exit(MEM_ERROR);
	}

	strncpy(curFunction->name, name, strlen(name));

	return;
}

void setFunctionTypeName(functionSymbolTable * curFunction, char * typeName)
{
	if(!(curFunction->typeName = calloc(strlen(typeName) + 1, 1))) {
		fprintf(stderr, "Error: Could not allocate memory for function type name!\n");
		exit(MEM_ERROR);
	}

	strncpy(curFunction->typeName, typeName, strlen(typeName));

	return;
}

void addStatementToFunction(functionSymbolTable * curFunction, statement * curStatement)
{
	if(!curFunction->statements) {
		curFunction->statements = curFunction->lastStatement = curStatement;
	} else {
		curFunction->lastStatement->nextStatement = curStatement;
		curFunction->lastStatement = curStatement;
	}

	return;
}

/* Functions for setting properties of variables */

void setVariableName(variableSymbol * curVariable, char * name)
{
	if(!(curVariable->name = calloc(strlen(name) + 1, 1))) {
		fprintf(stderr, "Error: Could not allocate memory for variable name!\n");
		exit(MEM_ERROR);
	}

	strncpy(curVariable->name, name, strlen(name));

	return;
}

void setVariableTypeName(variableSymbol * curVariable, char * typeName)
{
	if(!(curVariable->typeName = calloc(strlen(typeName) + 1, 1))) {
		fprintf(stderr, "Error: Could not allocate memory for variable type name!\n");
		exit(MEM_ERROR);
	}

	strncpy(curVariable->typeName, typeName, strlen(typeName));

	return;
}

void setVariableInitialised(variableSymbol * curVariable)
{
	curVariable->initialised = true;
}

/* Post-parse symbol table operations */

void finaliseSymbolTables()
{
	for(classSymbolTable * curClass = classes.firstClass; curClass; curClass = curClass->nextClass)
		finaliseClass(curClass);

	return;
}

void finaliseClass(classSymbolTable * curClass)
{
	int staticOffset, fieldOffset;

	currentClass = curClass;
	staticOffset = 0;
	fieldOffset = 0;

	for(variableSymbol * curVariable = curClass->variables; curVariable; curVariable = curVariable->nextVariable) {
		if(curVariable->type == field) {
			finaliseClassVariable(curVariable, &fieldOffset);
		} else {
			finaliseClassVariable(curVariable, &staticOffset);
		}
	}

	for(functionSymbolTable * curFunction = curClass->functions; curFunction; curFunction = curFunction->nextFunction)
		finaliseFunction(curFunction);

	return;
}

void finaliseFunction(functionSymbolTable * curFunction)
{
	int offset;

	currentFunction = curFunction;

	verifyFunctionType(curFunction);

	if(curFunction->type == method)
		offset = 1;
	else
		offset = 0;

	for(variableSymbol * curArgument = curFunction->arguments; curArgument; curArgument = curArgument->nextVariable)
		finaliseFunctionArgument(curArgument, &offset);

	offset = 0;

	for(variableSymbol * curVariable = curFunction->variables; curVariable; curVariable = curVariable->nextVariable)
		finaliseFunctionVariable(curVariable, &offset);
}

void finaliseClassVariable(variableSymbol * curVariable, int * offset)
{
	curVariable->offset = (*offset)++;

	verifyVariableType(curVariable);
	
	for(variableSymbol * cur = currentClass->variables; cur; cur = cur->nextVariable)
		if(!strcmp(curVariable->name, cur->name) && (curVariable != cur))
			finalisationError("Variable names must be unique", curVariable->lineNum);

	return;
}

void finaliseFunctionArgument(variableSymbol * curArgument, int * offset)
{
	curArgument->initialised = true;
	curArgument->isArgument = true;
	curArgument->offset = (*offset)++;

	verifyVariableType(curArgument);

	for(variableSymbol * cur = currentFunction->variables; cur; cur = cur->nextVariable)
		if(!strcmp(curArgument->name, cur->name) && (curArgument != cur))
			finalisationError("Variable names must be unique", curArgument->lineNum);

	for(variableSymbol * cur = currentFunction->arguments; cur; cur = cur->nextVariable)
		if(!strcmp(curArgument->name, cur->name) && (curArgument != cur))
			finalisationError("Variable names must be unique", curArgument->lineNum);

	return;
}

void finaliseFunctionVariable(variableSymbol * curVariable, int * offset)
{
	/* We don't need to do any initialisations for false values since the memory was allocated with calloc() and is already zero'd */
	curVariable->offset = (*offset)++;

	verifyVariableType(curVariable);

	for(variableSymbol * cur = currentFunction->variables; cur; cur = cur->nextVariable)
		if(!strcmp(curVariable->name, cur->name) && (curVariable != cur))
			finalisationError("Variable names must be unique", curVariable->lineNum);

	for(variableSymbol * cur = currentFunction->arguments; cur; cur = cur->nextVariable)
		if(!strcmp(curVariable->name, cur->name) && (curVariable != cur))
			finalisationError("Variable names must be unique", curVariable->lineNum);

	return;
}

void verifyVariableType(variableSymbol * curVariable)
{
	if(!strcmp(curVariable->typeName, "int") || !strcmp(curVariable->typeName, "boolean") || !strcmp(curVariable->typeName, "char")) {
		curVariable->construction = primitive;
	} else if(!strcmp(curVariable->typeName, "Array")) {
		curVariable->construction = array;
	} else {
		for(classSymbolTable * curClass = classes.firstClass; curClass; curClass = curClass->nextClass) {
			if(!strcmp(curVariable->typeName, curClass->name)) {
				curVariable->typeClass = curClass;
				break;
			}
		}

		if(!curVariable->typeClass)
			finalisationError("Function type does not exist", curVariable->lineNum);

		curVariable->construction = structure;
	}

	return;
}

void verifyFunctionType(functionSymbolTable * curFunction)
{
	if(strcmp(curFunction->typeName, "int") && strcmp(curFunction->typeName, "boolean") && strcmp(curFunction->typeName, "char") && strcmp(curFunction->typeName, "void") && strcmp(curFunction->typeName, "Array")) {
		for(classSymbolTable * curClass = classes.firstClass; curClass; curClass = curClass->nextClass) {
			if(!strcmp(curFunction->typeName, curClass->name)) {
				curFunction->typeClass = curClass;
				break;
			}
		}

		if(!curFunction->typeClass) {
			finalisationError("Function type does not exist", curFunction->lineNum);
		}
	}

	return;
}

/* Utility functions for code generation and semantic analysis */

variableSymbol * lookupClassVariable(classSymbolTable * curClass, char * variableName)
{
	for(variableSymbol * curVariable = curClass->variables; curVariable; curVariable = curVariable->nextVariable)
		if(!strcmp(curVariable->name, variableName))
			return curVariable;

	return NULL;
}

functionSymbolTable * lookupClassFunction(classSymbolTable * curClass, char * functionName)
{
	for(functionSymbolTable * curFunction = curClass->functions; curFunction; curFunction = curFunction->nextFunction)
		if(!strcmp(curFunction->name, functionName))
			return curFunction;

	return NULL;
}

variableSymbol * lookupFunctionVariable(functionSymbolTable * curFunction, char * variableName)
{
	for(variableSymbol * curVariable = curFunction->variables; curVariable; curVariable = curVariable->nextVariable)
		if(!strcmp(curVariable->name, variableName))
			return curVariable;

	for(variableSymbol * curArgument = curFunction->arguments; curArgument; curArgument = curArgument->nextVariable)
		if(!strcmp(curArgument->name, variableName))
			return curArgument;

	return NULL;
}

/* Cleanup functions */

void freeClasses()
{
	for(classSymbolTable * curClass = classes.firstClass; curClass; curClass = freeClass(curClass))
		;

	return;
}

classSymbolTable * freeClass(classSymbolTable * curClass)
{
	classSymbolTable * nextClass = curClass->nextClass;

	for(variableSymbol * curVariable = curClass->variables; curVariable; curVariable = freeVariable(curVariable))
		;

	for(functionSymbolTable * curFunction = curClass->functions; curFunction; curFunction = freeFunction(curFunction))
		;

	free(curClass->name);
	free(curClass);

	return nextClass;
}

functionSymbolTable * freeFunction(functionSymbolTable * curFunction)
{
	functionSymbolTable * nextFunction = curFunction->nextFunction;

	for(variableSymbol * curVariable = curFunction->variables; curVariable; curVariable = freeVariable(curVariable))
		;

	for(variableSymbol * curArgument = curFunction->arguments; curArgument; curArgument = freeVariable(curArgument))
		;

	freeStatement(curFunction->statements);
	free(curFunction->typeName);
	free(curFunction->name);
	free(curFunction);

	return nextFunction;
}

variableSymbol * freeVariable(variableSymbol * curVariable)
{
	variableSymbol * nextVariable = curVariable->nextVariable;

	free(curVariable->typeName);
	free(curVariable->name);
	free(curVariable);

	return nextVariable;
}