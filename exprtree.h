#ifndef EXPRTREE_H
#define EXPRTREE_H

#include <stdio.h>

#define MAX_NAME_LEN 64
#define MAX_STRING_LABEL_LEN 32
#define MAX_STRING_LITERAL_LEN 256
#define MAX_STRING_COUNT 256
#define MAX_FUNCTION_PARAMS 32

struct tnode{

    int val;
    int type;
    char varname[MAX_NAME_LEN];
    char op;
    
    char strval[MAX_STRING_LABEL_LEN];
    
    struct tnode *left;
    struct tnode *middle;
    struct tnode *right;
};

struct tnode* makeLeafNode(int n);
struct tnode* makeVarNode(char *name);
struct tnode* makeArrayNode(char *name, struct tnode *index);
struct tnode* makeArrayDeclNode(char *name, int size);
struct tnode* makeFuncNode(char *name, struct tnode *params, struct tnode *body, struct tnode *retExpr);
struct tnode* makeCallNode(char *name, struct tnode *args);
struct tnode* makeOperatorNode(char op, struct tnode* l, struct tnode* r);
struct tnode* makeIfNode(struct tnode* cond, struct tnode* thenpart, struct tnode* elsepart);
struct tnode* makeReadNode(char *name);
void printAst(struct tnode *t, FILE *out, int indent);
void freeTree(struct tnode *t);
void emitFunctionDefinitions(struct tnode *t);


int codeGen(struct tnode *t);

#endif
