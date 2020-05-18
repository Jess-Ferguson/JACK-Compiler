#ifndef JSYM_H
#define JSYM_H

#include <stdbool.h>

#include "../include/jlex.h"
#include "../include/jparse.h"

typedef enum functionTypes { constructor, method, func } functionType;
typedef enum variableTypes { variable, field, statik } variableType;
typedef enum variableConstructions { primitive, array, structure } variableConstruction;

typedef struct variableSymbol {
	bool isArgument;
	bool initialised;
	char * name;
	char * typeName;
	int offset;
	int lineNum;
	struct classSymbolTable * typeClass;
	struct variableSymbol * nextVariable;
	variableType type;
	variableConstruction construction;
} variableSymbol;

typedef struct functionSymbolTable {
	char * name;
	char * typeName;
	int argumentCount;
	int variableCount;
	int offset;
	int lineNum;
	struct functionSymbolTable * nextFunction;
	struct classSymbolTable * typeClass;
	struct statement * statements;
	struct statement * lastStatement;
	functionType type;
	variableSymbol * arguments;
	variableSymbol * lastArgument;
	variableSymbol * variables;
	variableSymbol * lastVariable;
} functionSymbolTable;

typedef struct classSymbolTable {
	char * name;
	int staticCount;
	int fieldCount;
	int functionCount;
	int lineNum;
	struct classSymbolTable * nextClass;
	variableSymbol * variables;
	variableSymbol * lastVariable;
	functionSymbolTable * functions;
	functionSymbolTable * lastFunction;
} classSymbolTable;

typedef struct classList {
	classSymbolTable * firstClass;
	classSymbolTable * lastClass;
} classList;

/* Functions for handling errors in the semantic analysis phase */

void semanticWarning(const char * warning);
void semanticError(const char * error);
void finalisationError(const char * error, int lineNum);

/* Functions for creating new symbols and symbol tables */

classSymbolTable * newClassSymbolTable(char * name);
functionSymbolTable * newFunctionSymbolTable();
variableSymbol * newVariableSymbol();

/* Functions for adding symbols to a class */

void addFunctionToClass(classSymbolTable * curClass, functionSymbolTable * curFunction);
void addVariableToClass(classSymbolTable * curClass, variableSymbol * curVariable);

/* Functions for adding variables to functions */

void addArgumentToFunction(functionSymbolTable * curFunction, variableSymbol * curVariable);
void addVariableToFunction(functionSymbolTable * curFunction, variableSymbol * curVariable);

/* Functions for setting properties of functions */

void setFunctionName(functionSymbolTable * curFunction, char * name);
void setFunctionTypeName(functionSymbolTable * curFunction, char * typeName);
void addStatementToFunction(functionSymbolTable * curFunction, struct statement * curStatement);
void incrementFunctionArgumentCount(functionSymbolTable * curFunction);
void incrementFunctionVariableCount(functionSymbolTable * curFunction);

/* Functions for setting properties of variables */

void setVariableName(variableSymbol * curVariable, char * name);
void setVariableTypeName(variableSymbol * curVariable, char * typeName);
void setVariableInitialised(variableSymbol * curVariable);

/* Functions for finalising the symbol tables */

void finaliseSymbolTables();
void finaliseClass(classSymbolTable * curClass);
void finaliseFunction(functionSymbolTable * curFunction);
void finaliseClassVariable(variableSymbol * curVariable, int * offset);
void finaliseFunctionArgument(variableSymbol * curArgument, int * offset);
void finaliseFunctionVariable(variableSymbol * curVariable, int * offset);

/* Functions which assist in the finalisation of the symbol table */

void verifyVariableType(variableSymbol * curVariable);
void verifyFunctionType(functionSymbolTable * curFunction);

/* Functions which assist in the semantic analysis/code generation phase */

functionSymbolTable * lookupClassFunction(classSymbolTable * curClass, char * functionName);
variableSymbol * lookupClassVariable(classSymbolTable * curClass, char * variableName);
variableSymbol * lookupFunctionVariable(functionSymbolTable * curFunction, char * Variablename);

/* Functions for cleaning up the symbol table and parse tree */

void freeClasses();
classSymbolTable * freeClass(classSymbolTable * curClass);
functionSymbolTable * freeFunction(functionSymbolTable * curFunction);
variableSymbol * freeVariable(variableSymbol * curVariable);

#endif