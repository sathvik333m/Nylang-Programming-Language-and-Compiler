#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "symboltable.h"

#define MAX_SCOPE_DEPTH 128
#define MAX_SCOPE_SYMBOLS 256

struct symbol *head=NULL;
int memory=4096;

struct scoped_symbol{
    char name[MAX_NAME_LEN];
    int type;
};

struct semantic_scope{
    struct scoped_symbol symbols[MAX_SCOPE_SYMBOLS];
    int count;
};

static struct semantic_scope scopeStack[MAX_SCOPE_DEPTH];
static int scopeTop = -1;

static void copyName(char *dest, const char *src)
{
    if(strlen(src) >= MAX_NAME_LEN){
        fprintf(stderr, "Identifier too long: %s\n", src);
        exit(1);
    }
    snprintf(dest, MAX_NAME_LEN, "%s", src);
}

static void copyScopeLabel(char *dest, const char *src)
{
    if(strlen(src) >= MAX_SCOPE_LABEL_LEN){
        fprintf(stderr, "Scope label too long: %s\n", src);
        exit(1);
    }
    snprintf(dest, MAX_SCOPE_LABEL_LEN, "%s", src);
}

static const char *symbolTypeName(int type)
{
    switch(type){
        case TYPE_INT:
            return "int";
        case TYPE_STRING:
            return "string";
        case TYPE_INT_ARRAY:
            return "int_array";
        default:
            return "unknown";
    }
}

static int intWidth(int value)
{
    int width = 1;

    if(value < 0){
        width++;
        value = -value;
    }

    while(value >= 10){
        value /= 10;
        width++;
    }

    return width;
}

static void printDivider(FILE *out,
                         int nameWidth,
                         int typeWidth,
                         int bindingWidth,
                         int sizeWidth,
                         int scopeWidth,
                         int depthWidth)
{
    int i;
    int widths[6];

    widths[0] = nameWidth;
    widths[1] = typeWidth;
    widths[2] = bindingWidth;
    widths[3] = sizeWidth;
    widths[4] = scopeWidth;
    widths[5] = depthWidth;

    fputc('+', out);
    for(i = 0; i < 6; i++){
        int j;
        for(j = 0; j < widths[i] + 2; j++){
            fputc('-', out);
        }
        fputc('+', out);
    }
    fputc('\n', out);
}

void install(char *name, int type){

    struct symbol *temp=head;

    while(temp){
        if(strcmp(temp->name,name)==0)
            return;
        temp=temp->next;
    }

    temp=malloc(sizeof(struct symbol));

    copyName(temp->name, name);
    temp->type = type;
    temp->size = (type == TYPE_INT_ARRAY) ? 100 : 1;
    temp->binding = memory++;
    temp->scope_depth = -1;
    temp->scope[0] = '\0';

    temp->next=head;
    head=temp;
}

void installArray(char *name, int size){

    struct symbol *temp=head;

    while(temp){
        if(strcmp(temp->name,name)==0)
            return;
        temp=temp->next;
    }

    temp=malloc(sizeof(struct symbol));

    copyName(temp->name, name);
    temp->type = TYPE_INT_ARRAY;
    temp->size = size;
    temp->binding = memory++;
    temp->scope_depth = -1;
    temp->scope[0] = '\0';

    temp->next=head;
    head=temp;
}


int getType(char *name){

    struct symbol *temp=head;

    while(temp){
        if(strcmp(temp->name,name)==0)
            return temp->type;

        temp=temp->next;
    }

    return -1;
}


int lookup(char *name){

    struct symbol *temp=head;

    while(temp){

        if(strcmp(temp->name,name)==0)
            return temp->binding;

        temp=temp->next;
    }

    return -1;
}

int getSize(char *name){

    struct symbol *temp=head;

    while(temp){
        if(strcmp(temp->name,name)==0)
            return temp->size;
        temp=temp->next;
    }

    return -1;
}



void printSymbolTable(){

    struct symbol *temp=head;

    while(temp){
       
         if(temp->type == TYPE_INT)
               printf("%s dq 0\n", temp->name);
         else if(temp->type == TYPE_INT_ARRAY)
               printf("%s times %d dq 0\n", temp->name, temp->size);
         else
               printf("%s times 100 db 0\n", temp->name);
       
        temp=temp->next;
    }
}

void dumpSymbolTable(FILE *out){

    struct symbol *temp=head;
    int nameWidth = (int)strlen("Name");
    int typeWidth = (int)strlen("Type");
    int bindingWidth = (int)strlen("Binding");
    int sizeWidth = (int)strlen("Size");
    int scopeWidth = (int)strlen("Scope");
    int depthWidth = (int)strlen("Depth");

    while(temp){
        int width;
        const char *typeName = symbolTypeName(temp->type);
        const char *scopeName = temp->scope[0] == '\0' ? "global" : temp->scope;
        int depth = temp->scope_depth < 0 ? 0 : temp->scope_depth;

        width = (int)strlen(temp->name);
        if(width > nameWidth) nameWidth = width;

        width = (int)strlen(typeName);
        if(width > typeWidth) typeWidth = width;

        width = intWidth(temp->binding);
        if(width > bindingWidth) bindingWidth = width;

        width = intWidth(temp->size);
        if(width > sizeWidth) sizeWidth = width;

        width = (int)strlen(scopeName);
        if(width > scopeWidth) scopeWidth = width;

        width = intWidth(depth);
        if(width > depthWidth) depthWidth = width;

        temp = temp->next;
    }

    if(head == NULL){
        fprintf(out, "(symbol table is empty)\n");
        return;
    }

    printDivider(out, nameWidth, typeWidth, bindingWidth, sizeWidth, scopeWidth, depthWidth);
    fprintf(out, "| %-*s | %-*s | %*s | %*s | %-*s | %*s |\n",
            nameWidth, "Name",
            typeWidth, "Type",
            bindingWidth, "Binding",
            sizeWidth, "Size",
            scopeWidth, "Scope",
            depthWidth, "Depth");
    printDivider(out, nameWidth, typeWidth, bindingWidth, sizeWidth, scopeWidth, depthWidth);

    temp = head;
    while(temp){
        fprintf(out, "| %-*s | %-*s | %*d | %*d | %-*s | %*d |\n",
                nameWidth, temp->name,
                typeWidth, symbolTypeName(temp->type),
                bindingWidth, temp->binding,
                sizeWidth, temp->size,
                scopeWidth, temp->scope[0] == '\0' ? "global" : temp->scope,
                depthWidth, temp->scope_depth < 0 ? 0 : temp->scope_depth);
        temp=temp->next;
    }

    printDivider(out, nameWidth, typeWidth, bindingWidth, sizeWidth, scopeWidth, depthWidth);
}

void freeSymbolTable(void){

    struct symbol *temp = head;
    struct symbol *next;

    while(temp){
        next = temp->next;
        free(temp);
        temp = next;
    }

    head = NULL;
    memory = 4096;
}

static char annotateScopeStack[MAX_SCOPE_DEPTH][MAX_SCOPE_LABEL_LEN];
static int annotateScopeTop = 0;

static void resetAnnotateScopes(void)
{
    annotateScopeTop = 0;
    copyScopeLabel(annotateScopeStack[0], "global");
}

static void pushAnnotateScope(const char *label)
{
    char combined[MAX_SCOPE_LABEL_LEN];

    if(annotateScopeTop >= MAX_SCOPE_DEPTH - 1){
        fprintf(stderr, "Annotation scope nesting too deep\n");
        exit(1);
    }

    if(snprintf(combined, sizeof(combined), "%s/%s",
                annotateScopeStack[annotateScopeTop], label) >= (int)sizeof(combined)){
        fprintf(stderr, "Annotation scope path too long\n");
        exit(1);
    }

    annotateScopeTop++;
    copyScopeLabel(annotateScopeStack[annotateScopeTop], combined);
}

static void popAnnotateScope(void)
{
    if(annotateScopeTop > 0){
        annotateScopeTop--;
    }
}

static struct symbol *findSymbol(const char *name)
{
    struct symbol *temp = head;

    while(temp){
        if(strcmp(temp->name, name) == 0){
            return temp;
        }
        temp = temp->next;
    }

    return NULL;
}

static void annotateSymbolIfUnset(const char *name)
{
    struct symbol *sym = findSymbol(name);

    if(sym == NULL || sym->scope_depth >= 0){
        return;
    }

    sym->scope_depth = annotateScopeTop;
    copyScopeLabel(sym->scope, annotateScopeStack[annotateScopeTop]);
}

static void annotateStatementScopes(struct tnode *t)
{
    if(t == NULL){
        return;
    }

    if(t->op == 'S'){
        annotateStatementScopes(t->left);
        annotateStatementScopes(t->right);
        return;
    }

    if(t->op == '='){
        if(t->left != NULL){
            if(t->left->type == 4){
                annotateSymbolIfUnset(t->left->varname);
            }
            else if(t->left->type == 1){
                annotateSymbolIfUnset(t->left->varname);
            }
        }
        return;
    }

    if(t->op == 'r' || t->op == 'D'){
        annotateSymbolIfUnset(t->varname);
        return;
    }

    if(t->op == 'I'){
        pushAnnotateScope("if");
        annotateStatementScopes(t->middle);
        popAnnotateScope();
        if(t->right != NULL){
            pushAnnotateScope("else");
            annotateStatementScopes(t->right);
            popAnnotateScope();
        }
        return;
    }

    if(t->op == 'W'){
        pushAnnotateScope("while");
        annotateStatementScopes(t->right);
        popAnnotateScope();
        return;
    }

    if(t->op == 'F'){
        pushAnnotateScope("for");
        annotateStatementScopes(t->left);
        if(t->middle != NULL){
            annotateStatementScopes(t->middle->left);
            annotateStatementScopes(t->middle->right);
        }
        popAnnotateScope();
        return;
    }
}

static void annotateFunctionScopes(struct tnode *t)
{
    if(t == NULL){
        return;
    }

    if(t->op == 'S'){
        annotateFunctionScopes(t->left);
        annotateFunctionScopes(t->right);
        return;
    }

    if(t->op != 'Q'){
        return;
    }

    pushAnnotateScope(t->varname);
    annotateStatementScopes(t->middle);
    popAnnotateScope();
}

void annotateSymbolScopes(struct tnode *functionRoot, struct tnode *programRoot)
{
    struct symbol *temp = head;

    resetAnnotateScopes();

    while(temp){
        temp->scope_depth = -1;
        temp->scope[0] = '\0';
        temp = temp->next;
    }

    annotateFunctionScopes(functionRoot);
    annotateStatementScopes(programRoot);
}

void resetScopeTable(void)
{
    scopeTop = -1;
}

void pushScope(void)
{
    if(scopeTop < MAX_SCOPE_DEPTH - 1){
        scopeTop++;
        scopeStack[scopeTop].count = 0;
    }
}

void popScope(void)
{
    if(scopeTop >= 0){
        scopeTop--;
    }
}

int lookupCurrentScopeType(const char *name)
{
    int i;

    if(scopeTop < 0){
        return -1;
    }

    for(i = scopeStack[scopeTop].count - 1; i >= 0; i--){
        if(strcmp(scopeStack[scopeTop].symbols[i].name, name) == 0){
            return scopeStack[scopeTop].symbols[i].type;
        }
    }

    return -1;
}

int lookupVisibleScopeType(const char *name)
{
    int i;
    int j;

    for(i = scopeTop; i >= 0; i--){
        for(j = scopeStack[i].count - 1; j >= 0; j--){
            if(strcmp(scopeStack[i].symbols[j].name, name) == 0){
                return scopeStack[i].symbols[j].type;
            }
        }
    }

    return -1;
}

int declareInCurrentScope(const char *name, int type)
{
    if(scopeTop < 0 || lookupCurrentScopeType(name) != -1){
        return -1;
    }

    if(scopeStack[scopeTop].count >= MAX_SCOPE_SYMBOLS){
        return -1;
    }

    copyName(scopeStack[scopeTop].symbols[scopeStack[scopeTop].count].name, name);
    scopeStack[scopeTop].symbols[scopeStack[scopeTop].count].type = type;
    scopeStack[scopeTop].count++;
    return 0;
}
