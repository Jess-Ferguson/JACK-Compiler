#ifndef JPARSE_H
#define JPARSE_H

#include <stdbool.h>

#include "../include/jlex.h" /* For the token variable type */
#include "../include/jsym.h" /* For the expression variable type */

/* Type definitions for building the parse tree */

typedef enum statementTypes { ifStatement, doStatement, whileStatement, varStatement, letStatement, returnStatement } statementType;
typedef enum constantTypes { integerType, stringType, keywordType } constantType; 
typedef enum termTypes { funcCall, expr, constant, reference, unaryTerm, arrayReference } termType;

typedef struct functionCall {
	char * actionName;
	struct expression ** expressionList;
	unsigned int expressionCount;
} functionCall;

typedef struct term {
	termType type;

	union {
		struct {
			constantType constantType;
			char * constantTerm;
		}; /* Constants */

		char * variableName; /* References */
		struct expression * expr; /* Expression */
		functionCall * call; /* Function call */ 

		struct {
			char operator;
			struct term * term;
		}; /* Unary terms */

		struct {
			char * arrayName;
			struct expression * indexExpression;
		}; /* Array reference */
	};
} term;

typedef struct type{
	char * typeName;
	struct type * nextType;
} type;

typedef struct expression {
	char * operators;
	term ** terms;
	unsigned int operatorCount;
	unsigned int termCount;
} expression;

typedef struct statement {
	statementType type;

	union {
		struct {
			expression * ifCondition;
			struct statement * ifStatements;
			struct statement * lastIfStatement;
			struct statement * elseStatements;
			struct statement * lastElseStatement;
		}; /* if statement */

		struct {
			expression * whileCondition;
			struct statement * whileStatements;
			struct statement * lastWhileStatement;
		}; /* while statement */

		struct {
			char * target;
			expression * indexExpression;
			expression * expression;
		}; /* let statement */

		functionCall * call; /* Do statement */
		expression * returnExpression; /* return statement */
	};

	int lineNum;
	struct statement * nextStatement;
} statement;

/* Recursive decent functions */

void parseClass();
void parseClassVarDeclaration();
void parseSubroutineDeclaration();
void parseParamList();
void parseSubroutineBody();
statement * parseStatement();
statement * parseReturnStatement();
statement * parseDoStatement();
statement * parseWhileStatement();
statement * parseIfStatement();
statement * parseLetStatement();
statement * parseVarDeclarStatement();
functionCall * parseSubroutineCall();
void parseExpressionList(functionCall * call);
expression * parseExpression();
term * parseTerm();
void parseType();

/* For creating and modifying new statements */

statement * newStatement();
statement * newLetStatement();
statement * newIfStatement();
statement * newReturnStatement();
statement * newDoStatement();
statement * newWhileStatement();

/* For creating and modifying new expressions */

expression * newExpression();
void addOperator(expression * curExpression, char operator);
void addTerm(expression * curExpression, term * curTerm);

/* For creating and modifying new terms */

term * newTerm();
void addConst(term * curTerm, char * constant);

/* For cleanup after the parse tree is no longer needed */

void freeStatement(statement * currentStatement);
void freeExpression(expression * currentExpression);
void freeTerm(term * currentTerm);

/* Handling of errors and tokens during the parsing phase */

void syntaxError(char * expected, token currToken);
void syntaxOkay(token currToken);

#endif