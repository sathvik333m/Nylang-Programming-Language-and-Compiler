#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <stdio.h>
#include "exprtree.h"

#define TYPE_INT 0
#define TYPE_STRING 1
#define TYPE_INT_ARRAY 2
#define MAX_SCOPE_LABEL_LEN 128

struct symbol{
    char name[MAX_NAME_LEN];
    int binding;
    int type;   // TYPE_INT, TYPE_STRING, TYPE_INT_ARRAY
    int size;
    int scope_depth;
    char scope[MAX_SCOPE_LABEL_LEN];
    struct symbol *next;
};

void install(char *name, int type);
void installArray(char *name, int size);

int getType(char *name);
int getSize(char *name);

int lookup(char *name);

void printSymbolTable();
void dumpSymbolTable(FILE *out);
void freeSymbolTable(void);

void annotateSymbolScopes(struct tnode *functionRoot, struct tnode *programRoot);

void resetScopeTable(void);
void pushScope(void);
void popScope(void);
int lookupCurrentScopeType(const char *name);
int lookupVisibleScopeType(const char *name);
int declareInCurrentScope(const char *name, int type);
#endif
