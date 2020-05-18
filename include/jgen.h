#ifndef JGEN_H
#define JGEN_H

#include <stdbool.h>

#include "../include/jsym.h"
#include "../include/jparse.h"

void generateCode();
void processClass(classSymbolTable * currentClass);
void processFunction(functionSymbolTable * currentFunction);
bool processStatements(statement * currentStatement);
bool processIfStatement(statement * currentStatement);
void processLetStatement(statement * currentStatement);
void processWhileStatement(statement * currentStatement);
void processReturnStatement(statement * currentStatement);
void processDoStatement(statement * currentStatement);
char * processExpression(expression * currentExpression);
void processOperator(char operator);
char * processTerm(term * curTerm);
char * processFunctionCall(functionCall * call);

#endif