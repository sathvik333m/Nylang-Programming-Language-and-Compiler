#include <stdio.h>
#include <string.h>
#include "semantic.h"
#include "symboltable.h"

#define SEM_TYPE_INT TYPE_INT
#define SEM_TYPE_STRING TYPE_STRING
#define SEM_TYPE_INT_ARRAY TYPE_INT_ARRAY
#define SEM_TYPE_VOID 3
#define SEM_TYPE_ERROR -1
#define MAX_FUNCTIONS 128

static int semanticErrors = 0;

struct function_info{
    char name[MAX_NAME_LEN];
    int paramCount;
    int returnType;
    struct tnode *params;
    struct tnode *body;
    struct tnode *retExpr;
};

static struct function_info functions[MAX_FUNCTIONS];
static int functionCount = 0;

static const char *semanticTypeName(int type)
{
    switch(type){
        case SEM_TYPE_INT: return "int";
        case SEM_TYPE_STRING: return "string";
        case SEM_TYPE_INT_ARRAY: return "int_array";
        case SEM_TYPE_VOID: return "void";
        default: return "error";
    }
}

static void reportSemanticError(FILE *out, const char *message)
{
    FILE *dest = out == NULL ? stderr : out;

    fprintf(dest, "Semantic error: %s\n", message);
    semanticErrors++;
}

static void reportTypeError(FILE *out, const char *context, int expected, int got)
{
    char message[256];

    snprintf(message, sizeof(message),
             "%s expects %s but got %s",
             context,
             semanticTypeName(expected),
             semanticTypeName(got));
    reportSemanticError(out, message);
}

static int inferExprType(struct tnode *t, FILE *out);
static int analyzeStatement(struct tnode *t, FILE *out, int loopDepth);
static int evaluateConstIntExpr(struct tnode *t, int *value);
static int countListNodes(struct tnode *t);
static struct function_info *lookupFunction(const char *name);
static void registerFunctions(struct tnode *t, FILE *out);
static void analyzeFunctions(FILE *out);
static void analyzeArgList(struct tnode *t, FILE *out);
static int declareScopedSymbol(const char *name, int type, FILE *out);
static int expectIntExpr(struct tnode *t, FILE *out, const char *context);
static int analyzeCallNode(struct tnode *t, FILE *out, int requireValue);

static int inferAssignedType(struct tnode *t, FILE *out)
{
    int valueType = inferExprType(t, out);

    if(valueType == SEM_TYPE_STRING){
        return SEM_TYPE_STRING;
    }

    if(valueType == SEM_TYPE_INT_ARRAY){
        return SEM_TYPE_INT_ARRAY;
    }

    return SEM_TYPE_INT;
}

static int countListNodes(struct tnode *t)
{
    if(t == NULL){
        return 0;
    }

    if(t->op == 'S'){
        return countListNodes(t->left) + countListNodes(t->right);
    }

    return 1;
}

static struct function_info *lookupFunction(const char *name)
{
    int i;

    for(i = 0; i < functionCount; i++){
        if(strcmp(functions[i].name, name) == 0){
            return &functions[i];
        }
    }

    return NULL;
}

static void registerFunctionNode(struct tnode *t, FILE *out)
{
    struct function_info *existing;
    int existingVarType;

    if(t == NULL){
        return;
    }

    if(t->op == 'S'){
        registerFunctionNode(t->left, out);
        registerFunctionNode(t->right, out);
        return;
    }

    if(t->op != 'Q'){
        return;
    }

    existing = lookupFunction(t->varname);
    if(existing != NULL){
        char message[256];
        snprintf(message, sizeof(message),
                 "function '%s' is already defined",
                 t->varname);
        reportSemanticError(out, message);
        return;
    }

    existingVarType = getType(t->varname);
    if(existingVarType != -1){
        char message[256];
        snprintf(message, sizeof(message),
                 "function '%s' conflicts with an existing variable or array name",
                 t->varname);
        reportSemanticError(out, message);
        return;
    }

    if(functionCount >= MAX_FUNCTIONS){
        reportSemanticError(out, "too many functions");
        return;
    }

    snprintf(functions[functionCount].name, MAX_NAME_LEN, "%s", t->varname);
    functions[functionCount].paramCount = countListNodes(t->left);
    functions[functionCount].returnType = t->right == NULL ? SEM_TYPE_VOID : SEM_TYPE_INT;
    functions[functionCount].params = t->left;
    functions[functionCount].body = t->middle;
    functions[functionCount].retExpr = t->right;
    functionCount++;
}

static void registerFunctions(struct tnode *t, FILE *out)
{
    functionCount = 0;
    registerFunctionNode(t, out);
}

static int declareParams(struct tnode *t, FILE *out)
{
    if(t == NULL){
        return SEM_TYPE_VOID;
    }

    if(t->op == 'S'){
        declareParams(t->left, out);
        declareParams(t->right, out);
        return SEM_TYPE_VOID;
    }

    if(t->type == 1){
        return declareScopedSymbol(t->varname, SEM_TYPE_INT, out);
    }

    return SEM_TYPE_VOID;
}

static void analyzeFunctions(FILE *out)
{
    int i;

    for(i = 0; i < functionCount; i++){
        pushScope();
        declareParams(functions[i].params, out);
        analyzeStatement(functions[i].body, out, 0);
        if(functions[i].returnType == SEM_TYPE_INT &&
           inferExprType(functions[i].retExpr, out) != SEM_TYPE_INT){
            char message[256];
            snprintf(message, sizeof(message),
                     "function '%s' must return int",
                     functions[i].name);
            reportSemanticError(out, message);
        }
        popScope();
    }
}

static int analyzeCallNode(struct tnode *t, FILE *out, int requireValue)
{
    struct function_info *fn;
    int argCount;
    char message[256];

    if(t == NULL){
        return SEM_TYPE_ERROR;
    }

    fn = lookupFunction(t->varname);
    if(fn == NULL){
        snprintf(message, sizeof(message),
                 "function '%s' is not defined",
                 t->varname);
        reportSemanticError(out, message);
        return SEM_TYPE_ERROR;
    }

    argCount = countListNodes(t->left);
    if(argCount != fn->paramCount){
        snprintf(message, sizeof(message),
                 "function '%s' expects %d argument(s) but got %d",
                 t->varname, fn->paramCount, argCount);
        reportSemanticError(out, message);
        return SEM_TYPE_ERROR;
    }

    analyzeArgList(t->left, out);

    if(requireValue && fn->returnType == SEM_TYPE_VOID){
        snprintf(message, sizeof(message),
                 "void function '%s' cannot be used in an expression",
                 t->varname);
        reportSemanticError(out, message);
        return SEM_TYPE_ERROR;
    }

    return fn->returnType;
}

static void analyzeArgList(struct tnode *t, FILE *out)
{
    if(t == NULL){
        return;
    }

    if(t->op == 'S'){
        analyzeArgList(t->left, out);
        analyzeArgList(t->right, out);
        return;
    }

    expectIntExpr(t, out, "function argument");
}

static int declareScopedSymbol(const char *name, int type, FILE *out)
{
    int currentType = lookupCurrentScopeType(name);
    char message[256];

    if(currentType != -1){
        snprintf(message, sizeof(message),
                 "'%s' is already declared in this block",
                 name);
        reportSemanticError(out, message);
        return SEM_TYPE_ERROR;
    }

    if(declareInCurrentScope(name, type) != 0){
        reportSemanticError(out, "semantic scope table is full");
        return SEM_TYPE_ERROR;
    }
    return SEM_TYPE_VOID;
}

static int analyzeBlock(struct tnode *t, FILE *out, int loopDepth)
{
    pushScope();
    analyzeStatement(t, out, loopDepth);
    popScope();
    return SEM_TYPE_VOID;
}

static int analyzeBinaryIntExpr(struct tnode *t, FILE *out, const char *name)
{
    int leftType = inferExprType(t->left, out);
    int rightType = inferExprType(t->right, out);

    if(leftType != SEM_TYPE_INT){
        reportTypeError(out, name, SEM_TYPE_INT, leftType);
        return SEM_TYPE_ERROR;
    }

    if(rightType != SEM_TYPE_INT){
        reportTypeError(out, name, SEM_TYPE_INT, rightType);
        return SEM_TYPE_ERROR;
    }

    return SEM_TYPE_INT;
}

static int expectIntExpr(struct tnode *t, FILE *out, const char *context)
{
    int type = inferExprType(t, out);

    if(type != SEM_TYPE_INT){
        reportTypeError(out, context, SEM_TYPE_INT, type);
        return SEM_TYPE_ERROR;
    }

    return SEM_TYPE_INT;
}

static int evaluateConstIntExpr(struct tnode *t, int *value)
{
    int leftValue;
    int rightValue;

    if(t == NULL){
        return 0;
    }

    if(t->type == 0){
        *value = t->val;
        return 1;
    }

    if(t->op == 'U' && evaluateConstIntExpr(t->left, &leftValue)){
        *value = -leftValue;
        return 1;
    }

    if(t->left == NULL || t->right == NULL){
        return 0;
    }

    if(!evaluateConstIntExpr(t->left, &leftValue) || !evaluateConstIntExpr(t->right, &rightValue)){
        return 0;
    }

    switch(t->op){
        case '+':
            *value = leftValue + rightValue;
            return 1;
        case '-':
            *value = leftValue - rightValue;
            return 1;
        case '*':
            *value = leftValue * rightValue;
            return 1;
        case '/':
            if(rightValue == 0) return 0;
            *value = leftValue / rightValue;
            return 1;
        case '%':
            if(rightValue == 0) return 0;
            *value = leftValue % rightValue;
            return 1;
        default:
            return 0;
    }
}

static void checkArrayBounds(struct tnode *arrayNode, FILE *out)
{
    int size;
    int indexValue;
    char message[256];

    if(arrayNode == NULL || arrayNode->type != 4){
        return;
    }

    size = getSize(arrayNode->varname);
    if(size <= 0){
        return;
    }

    if(evaluateConstIntExpr(arrayNode->left, &indexValue)){
        if(indexValue < 0 || indexValue >= size){
            snprintf(message, sizeof(message),
                     "array index %d is out of bounds for '%s' of size %d",
                     indexValue, arrayNode->varname, size);
            reportSemanticError(out, message);
        }
    }
}

static int inferExprType(struct tnode *t, FILE *out)
{
    int varType;

    if(t == NULL){
        return SEM_TYPE_VOID;
    }

    if(t->type == 0){
        return SEM_TYPE_INT;
    }

    if(t->type == 3){
        return SEM_TYPE_STRING;
    }

    if(t->type == 4){
        varType = lookupVisibleScopeType(t->varname);

        if(varType == -1){
            char message[256];
            snprintf(message, sizeof(message),
                     "array '%s' is used outside its scope or before assignment",
                     t->varname);
            reportSemanticError(out, message);
            return SEM_TYPE_ERROR;
        }

        if(varType != SEM_TYPE_INT_ARRAY){
            char message[256];
            snprintf(message, sizeof(message),
                     "'%s' is not an array",
                     t->varname);
            reportSemanticError(out, message);
            return SEM_TYPE_ERROR;
        }

        expectIntExpr(t->left, out, "array index");
        checkArrayBounds(t, out);
        return SEM_TYPE_INT;
    }

    if(t->type == 1){
        varType = lookupVisibleScopeType(t->varname);

        if(varType == -1){
            char message[256];
            snprintf(message, sizeof(message),
                     "variable '%s' is used outside its scope or before assignment",
                     t->varname);
            reportSemanticError(out, message);
            return SEM_TYPE_ERROR;
        }

        if(varType == SEM_TYPE_INT_ARRAY){
            char message[256];
            snprintf(message, sizeof(message),
                     "array '%s' must be indexed before use",
                     t->varname);
            reportSemanticError(out, message);
            return SEM_TYPE_ERROR;
        }

        return varType;
    }

    if(t->op == 'K'){
        return analyzeCallNode(t, out, 1);
    }

    switch(t->op){
        case 'R':
            return SEM_TYPE_INT;

        case 'U':
            return expectIntExpr(t->left, out, "unary '-'");

        case '!':
            return expectIntExpr(t->left, out, "'not'");

        case '+':
            return analyzeBinaryIntExpr(t, out, "'+'");
        case '-':
            return analyzeBinaryIntExpr(t, out, "'-'");
        case '*':
            return analyzeBinaryIntExpr(t, out, "'*'");
        case '/':
            return analyzeBinaryIntExpr(t, out, "'/'");
        case '%':
            return analyzeBinaryIntExpr(t, out, "'%'");
        case '<':
            return analyzeBinaryIntExpr(t, out, "'<'");
        case '>':
            return analyzeBinaryIntExpr(t, out, "'>'");
        case 'L':
            return analyzeBinaryIntExpr(t, out, "'<='");
        case 'G':
            return analyzeBinaryIntExpr(t, out, "'>='");
        case 'E':
            return analyzeBinaryIntExpr(t, out, "'=='");
        case 'N':
            return analyzeBinaryIntExpr(t, out, "'!='");
        case 'A':
            return analyzeBinaryIntExpr(t, out, "'and'");
        case 'O':
            return analyzeBinaryIntExpr(t, out, "'or'");

        default:
            return SEM_TYPE_ERROR;
    }
}

static int analyzeAssignment(struct tnode *t, FILE *out)
{
    int targetType = lookupVisibleScopeType(t->left->varname);
    int valueType = inferAssignedType(t->right, out);
    char message[256];

    if(lookupFunction(t->left->varname) != NULL){
        snprintf(message, sizeof(message),
                 "'%s' is already defined as a function name",
                 t->left->varname);
        reportSemanticError(out, message);
        return SEM_TYPE_ERROR;
    }

    if(t->left->type == 4){
        expectIntExpr(t->left->left, out, "array index");
        checkArrayBounds(t->left, out);

        if(targetType == -1){
            declareInCurrentScope(t->left->varname, SEM_TYPE_INT_ARRAY);
            targetType = SEM_TYPE_INT_ARRAY;
        }

        if(targetType != SEM_TYPE_INT_ARRAY){
            snprintf(message, sizeof(message),
                     "cannot index non-array variable '%s'",
                     t->left->varname);
            reportSemanticError(out, message);
            return SEM_TYPE_ERROR;
        }

        if(valueType != SEM_TYPE_INT){
            snprintf(message, sizeof(message),
                     "cannot assign %s to array element '%s[...]'",
                     semanticTypeName(valueType),
                     t->left->varname);
            reportSemanticError(out, message);
            return SEM_TYPE_ERROR;
        }

        return SEM_TYPE_VOID;
    }

    if(targetType == -1){
        declareInCurrentScope(t->left->varname, valueType);
        targetType = valueType;
    }

    if(targetType != valueType){
        snprintf(message, sizeof(message),
                 "cannot assign %s to variable '%s' of type %s",
                 semanticTypeName(valueType),
                 t->left->varname,
                 semanticTypeName(targetType));
        reportSemanticError(out, message);
        return SEM_TYPE_ERROR;
    }

    return SEM_TYPE_VOID;
}

static int analyzeStatement(struct tnode *t, FILE *out, int loopDepth)
{
    int condType;
    int targetType;
    int childStatus = SEM_TYPE_VOID;
    char message[256];

    if(t == NULL){
        return SEM_TYPE_VOID;
    }

    if(t->op == 'S'){
        analyzeStatement(t->left, out, loopDepth);
        analyzeStatement(t->right, out, loopDepth);
        return SEM_TYPE_VOID;
    }

    if(t->op == '='){
        return analyzeAssignment(t, out);
    }

    if(t->op == 'D'){
        if(lookupFunction(t->varname) != NULL){
            snprintf(message, sizeof(message),
                     "array '%s' conflicts with a function name",
                     t->varname);
            reportSemanticError(out, message);
            return SEM_TYPE_ERROR;
        }
        if(t->val <= 0){
            reportSemanticError(out, "array size must be greater than 0");
            return SEM_TYPE_ERROR;
        }
        return declareScopedSymbol(t->varname, SEM_TYPE_INT_ARRAY, out);
    }

    if(t->op == 'P' || t->op == 'Z'){
        inferExprType(t->left, out);
        return SEM_TYPE_VOID;
    }

    if(t->op == 'X'){
        return analyzeCallNode(t->left, out, 0);
    }

    if(t->op == 'r'){
        targetType = lookupVisibleScopeType(t->varname);

        if(lookupFunction(t->varname) != NULL){
            snprintf(message, sizeof(message),
                     "read target '%s' conflicts with a function name",
                     t->varname);
            reportSemanticError(out, message);
            return SEM_TYPE_ERROR;
        }

        if(targetType == -1){
            declareInCurrentScope(t->varname, SEM_TYPE_STRING);
            return SEM_TYPE_VOID;
        }

        if(targetType != SEM_TYPE_STRING){
            snprintf(message, sizeof(message),
                     "read(%s) stores a string, but '%s' is declared as int",
                     t->varname, t->varname);
            reportSemanticError(out, message);
            return SEM_TYPE_ERROR;
        }

        return SEM_TYPE_VOID;
    }

    if(t->op == 'I'){
        condType = inferExprType(t->left, out);
        if(condType != SEM_TYPE_INT){
            reportTypeError(out, "if condition", SEM_TYPE_INT, condType);
        }
        analyzeBlock(t->middle, out, loopDepth);
        if(t->right != NULL){
            analyzeBlock(t->right, out, loopDepth);
        }
        return SEM_TYPE_VOID;
    }

    if(t->op == 'W'){
        condType = inferExprType(t->left, out);
        if(condType != SEM_TYPE_INT){
            reportTypeError(out, "while condition", SEM_TYPE_INT, condType);
        }
        analyzeBlock(t->right, out, loopDepth + 1);
        return SEM_TYPE_VOID;
    }

    if(t->op == 'F'){
        pushScope();

        analyzeStatement(t->left, out, loopDepth);

        condType = inferExprType(t->right, out);
        if(condType != SEM_TYPE_INT){
            reportTypeError(out, "for condition", SEM_TYPE_INT, condType);
        }

        if(t->middle != NULL){
            analyzeBlock(t->middle->left, out, loopDepth + 1);
            analyzeStatement(t->middle->right, out, loopDepth + 1);
        }

        popScope();

        return SEM_TYPE_VOID;
    }

    if(t->op == 'B' || t->op == 'C'){
        if(loopDepth == 0){
            reportSemanticError(out, t->op == 'B'
                ? "'break' used outside of a loop"
                : "'continue' used outside of a loop");
            return SEM_TYPE_ERROR;
        }
        return SEM_TYPE_VOID;
    }

    childStatus = inferExprType(t, out);
    return childStatus == SEM_TYPE_ERROR ? SEM_TYPE_ERROR : SEM_TYPE_VOID;
}

int analyzeSemantics(struct tnode *functionRoot, struct tnode *root, FILE *out)
{
    semanticErrors = 0;
    resetScopeTable();
    registerFunctions(functionRoot, out);
    analyzeFunctions(out);
    pushScope();
    analyzeStatement(root, out, 0);
    popScope();

    if(out != NULL && semanticErrors == 0){
        fprintf(out, "Semantic analysis: success\n");
    }
    else if(out != NULL){
        fprintf(out, "Semantic analysis: failed with %d error(s)\n", semanticErrors);
    }

    return semanticErrors;
}
