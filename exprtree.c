#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "exprtree.h"
#include "symboltable.h"

int breakStack[100];
int continueStack[100];
int top=-1;

char stringLiterals[MAX_STRING_COUNT][MAX_STRING_LITERAL_LEN];
int stringCount = 0;

int label=0;
char currentFunctionParams[MAX_FUNCTION_PARAMS][MAX_NAME_LEN];
int currentFunctionParamCount = 0;

int getLabel(){ return label++; }

static void copyName(char *dest, const char *src)
{
    if(strlen(src) >= MAX_NAME_LEN){
        fprintf(stderr, "Identifier too long: %s\n", src);
        exit(1);
    }
    snprintf(dest, MAX_NAME_LEN, "%s", src);
}

static void pushLoopContext(int breakLabel, int continueLabel)
{
    if(top >= 99){
        fprintf(stderr, "Loop nesting too deep\n");
        exit(1);
    }

    top++;
    breakStack[top] = breakLabel;
    continueStack[top] = continueLabel;
}

static void popLoopContext(void)
{
    if(top < 0){
        fprintf(stderr, "Internal loop-context underflow\n");
        exit(1);
    }

    top--;
}

static void emitAlignedCall(const char *name)
{
    printf("sub rsp,8\n");
    printf("call %s\n", name);
    printf("add rsp,8\n");
}

static int getParamOffset(const char *name)
{
    int i;

    for(i = 0; i < currentFunctionParamCount; i++){
        if(strcmp(currentFunctionParams[i], name) == 0){
            return 16 + (i * 8);
        }
    }

    return -1;
}

static void emitArrayBoundsCheck(char *name)
{
    int okLabel = getLabel();
    int failLabel = getLabel();
    int size = getSize(name);

    printf("cmp rax,0\n");
    printf("jl L%d\n", failLabel);
    printf("cmp rax,%d\n", size);
    printf("jl L%d\n", okLabel);
    printf("L%d:\n", failLabel);
    printf("jmp oob_error\n");
    printf("L%d:\n", okLabel);
}

struct tnode* makeLeafNode(int n){

    struct tnode *t = malloc(sizeof(struct tnode));

    t->val=n;
    t->type=0;
    t->op=0;

    t->left=t->middle=t->right=NULL;

    return t;
}

struct tnode* makeVarNode(char *name){

    struct tnode *t = malloc(sizeof(struct tnode));

    t->type=1;
    t->op=0;
    copyName(t->varname, name);

    t->left=t->middle=t->right=NULL;

    return t;
}

struct tnode* makeArrayNode(char *name, struct tnode *index){

    struct tnode *t = malloc(sizeof(struct tnode));

    t->type=4;
    t->op=0;
    copyName(t->varname, name);

    t->left=index;
    t->middle=t->right=NULL;

    return t;
}

struct tnode* makeArrayDeclNode(char *name, int size){

    struct tnode *t = malloc(sizeof(struct tnode));

    t->type=2;
    t->op='D';
    t->val=size;
    copyName(t->varname, name);

    t->left=t->middle=t->right=NULL;

    return t;
}

struct tnode* makeFuncNode(char *name, struct tnode *params, struct tnode *body, struct tnode *retExpr){

    struct tnode *t = malloc(sizeof(struct tnode));

    t->type = 2;
    t->op = 'Q';
    copyName(t->varname, name);
    t->left = params;
    t->middle = body;
    t->right = retExpr;

    return t;
}

struct tnode* makeCallNode(char *name, struct tnode *args){

    struct tnode *t = malloc(sizeof(struct tnode));

    t->type = 2;
    t->op = 'K';
    copyName(t->varname, name);
    t->left = args;
    t->middle = t->right = NULL;

    return t;
}

struct tnode* makeOperatorNode(char op, struct tnode* l, struct tnode* r){

    struct tnode *t = malloc(sizeof(struct tnode));

    t->op=op;
    t->type=2;

    t->left=l;
    t->middle=NULL;
    t->right=r;

    return t;
}

struct tnode* makeIfNode(struct tnode* cond, struct tnode* thenpart, struct tnode* elsepart){
    struct tnode *t = malloc(sizeof(struct tnode));

    t->type= 2;
    t->op='I';

    t->left=cond;
    t->middle=thenpart;
    t->right=elsepart;

    return t;
}

struct tnode* makeStringNode(char *s){

    struct tnode *t = malloc(sizeof(struct tnode));

    if(stringCount >= MAX_STRING_COUNT){
        fprintf(stderr, "Too many string literals\n");
        exit(1);
    }

    if(strlen(s) >= MAX_STRING_LITERAL_LEN){
        fprintf(stderr, "String literal too long: %s\n", s);
        exit(1);
    }

    t->type = 3;
    t->op = 0;

    snprintf(t->strval, MAX_STRING_LABEL_LEN, "msg%d", stringCount);
    snprintf(stringLiterals[stringCount], MAX_STRING_LITERAL_LEN, "%s", s);

    stringCount++;

    t->left = t->middle = t->right = NULL;

    return t;
}

struct tnode* makeReadNode(char *name){

    struct tnode *t = malloc(sizeof(struct tnode));

    t->type = 2;
    t->op = 'r';

    copyName(t->varname, name);

    t->left = t->middle = t->right = NULL;

    return t;
}

static const char *nodeName(struct tnode *t)
{
    if(t == NULL) return "null";

    if(t->type == 0) return "Number";
    if(t->type == 1) return "Variable";
    if(t->type == 3) return "String";
    if(t->type == 4) return "ArrayAccess";

    switch(t->op){
        case 'S': return "StatementList";
        case '=': return "Assign";
        case 'P': return "Print";
        case 'Z': return "Println";
        case 'X': return "CallStatement";
        case 'r': return "ReadIntoVar";
        case 'R': return "ReadExpr";
        case 'I': return "If";
        case 'W': return "While";
        case 'F': return "For";
        case 'B': return "Break";
        case 'C': return "Continue";
        case 'D': return "ArrayDecl";
        case 'Q': return "Function";
        case 'K': return "Call";
        case 'U': return "UnaryMinus";
        case '!': return "Not";
        case '+': return "Add";
        case '-': return "Subtract";
        case '*': return "Multiply";
        case '/': return "Divide";
        case '%': return "Modulo";
        case '<': return "LessThan";
        case '>': return "GreaterThan";
        case 'L': return "LessEqual";
        case 'G': return "GreaterEqual";
        case 'E': return "Equal";
        case 'N': return "NotEqual";
        case 'A': return "And";
        case 'O': return "Or";
        default: return "Operator";
    }
}

static const char *stringLiteralForNode(struct tnode *t)
{
    int index = -1;

    if(t == NULL || t->type != 3) return "";

    if(sscanf(t->strval, "msg%d", &index) == 1 && index >= 0 && index < stringCount){
        return stringLiterals[index];
    }

    return "";
}

static void printTreePrefix(FILE *out, const int *branches, int depth)
{
    for(int i = 0; i < depth; i++){
        fputs(branches[i] ? "|   " : "    ", out);
    }
}

static void printAstNodeLine(struct tnode *t, FILE *out)
{
    if(t == NULL){
        fputs("(null)", out);
        return;
    }

    if(t->type == 0){
        fprintf(out, "Number value=%d", t->val);
        return;
    }

    if(t->type == 1){
        fprintf(out, "Variable name=%s", t->varname);
        return;
    }

    if(t->type == 4){
        fprintf(out, "ArrayAccess name=%s", t->varname);
        return;
    }

    if(t->type == 3){
        fprintf(out, "String label=%s literal=%s", t->strval, stringLiteralForNode(t));
        return;
    }

    if(t->op == 'r'){
        fprintf(out, "ReadIntoVar target=%s", t->varname);
        return;
    }

    if(t->op == 'D'){
        fprintf(out, "ArrayDecl name=%s size=%d", t->varname, t->val);
        return;
    }

    if(t->op == 'Q'){
        fprintf(out, "Function name=%s", t->varname);
        return;
    }

    if(t->op == 'K'){
        fprintf(out, "Call name=%s", t->varname);
        return;
    }

    fprintf(out, "%s", nodeName(t));
}

static int collectChildren(struct tnode *t, struct tnode **children, const char **labels)
{
    int count = 0;

    if(t == NULL){
        return 0;
    }

    if(t->op == '='){
        children[count] = t->left;
        labels[count++] = "target";
        children[count] = t->right;
        labels[count++] = "value";
        return count;
    }

    if(t->op == 'P' || t->op == 'Z'){
        children[count] = t->left;
        labels[count++] = "expr";
        return count;
    }

    if(t->op == 'X'){
        children[count] = t->left;
        labels[count++] = "call";
        return count;
    }

    if(t->op == 'Q'){
        if(t->left != NULL){
            children[count] = t->left;
            labels[count++] = "params";
        }
        if(t->middle != NULL){
            children[count] = t->middle;
            labels[count++] = "body";
        }
        if(t->right != NULL){
            children[count] = t->right;
            labels[count++] = "return";
        }
        return count;
    }

    if(t->op == 'K'){
        if(t->left != NULL){
            children[count] = t->left;
            labels[count++] = "args";
        }
        return count;
    }

    if(t->op == 'I'){
        children[count] = t->left;
        labels[count++] = "cond";
        children[count] = t->middle;
        labels[count++] = "then";
        if(t->right != NULL){
            children[count] = t->right;
            labels[count++] = "else";
        }
        return count;
    }

    if(t->op == 'W'){
        children[count] = t->left;
        labels[count++] = "cond";
        children[count] = t->right;
        labels[count++] = "body";
        return count;
    }

    if(t->op == 'F' && t->middle != NULL){
        children[count] = t->left;
        labels[count++] = "init";
        children[count] = t->right;
        labels[count++] = "cond";
        children[count] = t->middle->left;
        labels[count++] = "body";
        children[count] = t->middle->right;
        labels[count++] = "update";
        return count;
    }

    if(t->op == 'S'){
        children[count] = t->left;
        labels[count++] = "first";
        children[count] = t->right;
        labels[count++] = "next";
        return count;
    }

    if(t->op == 'U' || t->op == '!' || t->op == 'R'){
        if(t->left != NULL){
            children[count] = t->left;
            labels[count++] = "expr";
        }
        return count;
    }

    if(t->op == 'D'){
        return count;
    }

    if(t->type == 4){
        children[count] = t->left;
        labels[count++] = "index";
        return count;
    }

    if(t->left != NULL){
        children[count] = t->left;
        labels[count++] = "lhs";
    }

    if(t->middle != NULL){
        children[count] = t->middle;
        labels[count++] = "mid";
    }

    if(t->right != NULL){
        children[count] = t->right;
        labels[count++] = "rhs";
    }

    return count;
}

static void collectParamNames(struct tnode *t)
{
    if(t == NULL){
        return;
    }

    if(t->op == 'S'){
        collectParamNames(t->left);
        collectParamNames(t->right);
        return;
    }

    if(t->type == 1 && currentFunctionParamCount < MAX_FUNCTION_PARAMS){
        copyName(currentFunctionParams[currentFunctionParamCount], t->varname);
        currentFunctionParamCount++;
    }
}

void emitFunctionDefinitions(struct tnode *t)
{
    int savedParamCount;
    char savedParams[MAX_FUNCTION_PARAMS][MAX_NAME_LEN];
    int i;

    if(t == NULL){
        return;
    }

    if(t->op == 'S'){
        emitFunctionDefinitions(t->left);
        emitFunctionDefinitions(t->right);
        return;
    }

    if(t->op != 'Q'){
        return;
    }

    savedParamCount = currentFunctionParamCount;
    for(i = 0; i < savedParamCount; i++){
        copyName(savedParams[i], currentFunctionParams[i]);
    }

    currentFunctionParamCount = 0;
    collectParamNames(t->left);

    printf("F_%s:\n", t->varname);
    printf("push rbp\n");
    printf("mov rbp,rsp\n");

    codeGen(t->middle);
    if(t->right != NULL){
        codeGen(t->right);
        printf("pop rax\n");
    }
    else{
        printf("mov rax,0\n");
    }
    printf("mov rsp,rbp\n");
    printf("pop rbp\n");
    printf("ret\n");

    currentFunctionParamCount = savedParamCount;
    for(i = 0; i < savedParamCount; i++){
        copyName(currentFunctionParams[i], savedParams[i]);
    }
}

static void printAstRecursive(struct tnode *t, FILE *out, int depth, int *branches, const char *edgeLabel, int isLast)
{
    struct tnode *children[4];
    const char *labels[4];
    int childCount;

    if(depth > 0){
        printTreePrefix(out, branches, depth - 1);
        fprintf(out, "%s", isLast ? "`-- " : "|-- ");
        if(edgeLabel != NULL){
            fprintf(out, "%s: ", edgeLabel);
        }
    }

    printAstNodeLine(t, out);
    fputc('\n', out);

    childCount = collectChildren(t, children, labels);

    for(int i = 0; i < childCount; i++){
        branches[depth] = (i != childCount - 1);
        printAstRecursive(children[i], out, depth + 1, branches, labels[i], i == childCount - 1);
    }
}

void printAst(struct tnode *t, FILE *out, int indent)
{
    int branches[128] = {0};

    for(int i = 0; i < indent && i < 128; i++){
        branches[i] = 1;
    }

    if(t == NULL){
        fputs("(null)\n", out);
        return;
    }

    printAstRecursive(t, out, indent, branches, NULL, 1);
}





int codeGen(struct tnode *t)
{
    if(t==NULL) return -1;

    if(t->op=='S'){
        codeGen(t->left);
        
        codeGen(t->right);
        return -1;
    }

    if(t->op=='D'){
        return -1;
    }

    if(t->op=='X'){
        codeGen(t->left);
        printf("add rsp,8\n");
        return -1;
    }

    /* number */
    if(t->type==0){
        printf("mov rax,%d\n",t->val);
        printf("push rax\n");
        return -1;
    }

    /* variable load */
    if(t->type==1){
        int paramOffset = getParamOffset(t->varname);

        if(paramOffset != -1)
            printf("mov rax,[rbp+%d]\n", paramOffset);
        else
            printf("mov rax,[%s]\n",t->varname);
        printf("push rax\n");
        return -1;
    }

    /* array element load */
    if(t->type==4){
        codeGen(t->left);
        printf("pop rax\n");
        emitArrayBoundsCheck(t->varname);
        printf("mov rbx,[%s + rax*8]\n", t->varname);
        printf("push rbx\n");
        return -1;
    }

    /* assignment */
    if(t->op=='='){
         if(t->left == NULL){
             fprintf(stderr, "Internal error: assignment target is NULL\n");
             exit(1);
         }
         int leftType = getType(t->left->varname);

         if(t->left->type == 4){
            codeGen(t->left->left);
            codeGen(t->right);
            printf("pop rbx\n");
            printf("pop rax\n");
            emitArrayBoundsCheck(t->left->varname);
            printf("mov [%s + rax*8],rbx\n", t->left->varname);
            return -1;
         }

         // STRING ASSIGNMENT
         if(leftType == 1){

               if(t->right->type == 3){
                   codeGen(t->right);   // pushes address
                   printf("pop rsi\n"); // source
               }
               else if(t->right->type == 1 && getType(t->right->varname) == 1){
                   printf("lea rsi,[%s]\n", t->right->varname);
               }
               else{
                   return -1;
               }

               printf("lea rdi,[%s]\n", t->left->varname); // destination

               emitAlignedCall("strcpy");

               return -1;
         }

         // INT ASSIGNMENT
        codeGen(t->right);
        printf("pop rax\n");
        {
            int paramOffset = getParamOffset(t->left->varname);
            if(paramOffset != -1)
                printf("mov [rbp+%d],rax\n", paramOffset);
            else
                printf("mov [%s],rax\n",t->left->varname);
        }

        return -1;
     }
     
     
     if(t->op=='r'){
           printf("lea rdi,[%s]\n", t->varname);
           printf("call read_string_into\n");
 
          return -1;
    }
     
    
    if(t->type == 3){
        printf("lea rax, [%s]\n", t->strval);
        printf("push rax\n");
        return -1;
    }
    
    /*print*/
    if(t->op=='P' || t->op=='Z'){
         const char *intFmt = (t->op == 'Z') ? "fmtln" : "fmt";
         const char *strFmt = (t->op == 'Z') ? "fmtstrln" : "fmtstr";

         // STRING LITERAL
               if(t->left->type == 3){
            codeGen(t->left);
            printf("pop rsi\n");
                printf("lea rdi,[rel %s]\n", strFmt);
      }

    // VARIABLE
    else if(t->left->type == 1){

        int type = getType(t->left->varname);
        int paramOffset = getParamOffset(t->left->varname);

        if(type == TYPE_STRING){
            // string variable
            printf("lea rsi,[%s]\n", t->left->varname);
            printf("lea rdi,[rel %s]\n", strFmt);
        }
        else{
            // integer variable
            if(paramOffset != -1)
                printf("mov rax,[rbp+%d]\n", paramOffset);
            else
                printf("mov rax,[%s]\n", t->left->varname);
            printf("mov rsi,rax\n");
            printf("lea rdi,[rel %s]\n", intFmt);
        }
    }

    // EXPRESSIONS (default int)
    else{
        codeGen(t->left);
        printf("pop rsi\n");
        printf("lea rdi,[rel %s]\n", intFmt);
    }

    printf("xor rax,rax\n");
    emitAlignedCall("printf");

    return -1;
    }  
   

    /* IF */
    if(t->op=='I'){
        int l1=getLabel();
        int l2=getLabel();

        codeGen(t->left);

        printf("pop rax\n");
        printf("cmp rax,0\n");
        printf("je L%d\n",l1);

        codeGen(t->middle);
	printf("jmp L%d\n",l2);

        printf("L%d:\n",l1);

        if(t->right!=NULL)
            codeGen(t->right);

        printf("L%d:\n",l2);

        return -1;
    }

    /* WHILE */
    if(t->op=='W'){
    int l1=getLabel();
    int l2=getLabel();

    pushLoopContext(l2, l1);

    printf("L%d:\n",l1);

    codeGen(t->left);

    printf("pop rax\n");
    printf("cmp rax,0\n");
    printf("je L%d\n",l2);

    codeGen(t->right);

    printf("jmp L%d\n",l1);
    printf("L%d:\n",l2);

    popLoopContext();

    return -1;
    }
    
    if(t->op=='B'){
    if(top < 0){
    fprintf(stderr, "Internal break without loop context\n");
    exit(1);
    }
    printf("jmp L%d\n", breakStack[top]);
    return -1;
    }
    
    if(t->op=='C'){
    if(top < 0){
    fprintf(stderr, "Internal continue without loop context\n");
    exit(1);
    }
    printf("jmp L%d\n", continueStack[top]);
    return -1;
    }
    
    if(t->op=='!'){

    codeGen(t->left);

    printf("pop rax\n");
    printf("cmp rax,0\n");
    printf("sete al\n");
    printf("movzx rax,al\n");
    printf("push rax\n");

    return -1;
   }
   if(t->op=='R'){

    printf("call read_int_expr\n");
    printf("push rax\n");

    return -1;
    }

    if(t->op=='K'){
    struct tnode *args[128];
    int argCount = 0;
    int i;
    struct tnode *cur = t->left;

    while(cur != NULL){
        if(cur->op == 'S'){
            args[argCount++] = cur->right;
            cur = cur->left;
        }
        else{
            args[argCount++] = cur;
            break;
        }
    }

    for(i = 0; i < argCount / 2; i++){
        struct tnode *tmp = args[i];
        args[i] = args[argCount - 1 - i];
        args[argCount - 1 - i] = tmp;
    }

    for(i = argCount - 1; i >= 0; i--){
        codeGen(args[i]);
    }

    printf("call F_%s\n", t->varname);
    if(argCount > 0)
        printf("add rsp,%d\n", argCount * 8);
    printf("push rax\n");
    return -1;
    }
    if(t->op=='F'){

    int l1 = getLabel(); // start
    int l2 = getLabel(); // end
    int l3 = getLabel(); // update

    // init
    codeGen(t->left);

    // push loop context
    pushLoopContext(l2, l3);

    printf("L%d:\n", l1);

    // condition
    codeGen(t->right);
    printf("pop rax\n");
    printf("cmp rax,0\n");
    printf("je L%d\n", l2);

    // body (ONLY body, not update)
    codeGen(t->middle->left);

    // continue lands here
    printf("L%d:\n", l3);

    // update
    
    codeGen(t->middle->right);

    // loop
    printf("jmp L%d\n", l1);

    printf("L%d:\n", l2);

    popLoopContext();

    return -1;
}
    if(t->op=='U'){

    codeGen(t->left);

    printf("pop rax\n");
    printf("neg rax\n");
    printf("push rax\n");

    return -1;
    }

    /* arithmetic */

    codeGen(t->left);
    codeGen(t->right);
    
    printf("pop rbx\n");
    printf("pop rax\n");

    
    switch(t->op){

        case '+':
            printf("add rax,rbx\n");
            break;

        case '-':
            printf("sub rax,rbx\n");
            break;

        case '*':
            printf("imul rax,rbx\n");
            break;

        case '/':
            printf("cmp rbx,0\n");
            printf("je div_error\n");
            printf("cqo\n");
            printf("idiv rbx\n");
            break;
        case '%':
            printf("cmp rbx,0\n");
            printf("je div_error\n");
            printf("cqo\n");
            printf("idiv rbx\n");
            printf("mov rax,rdx\n");
            break;

        case '<':
            printf("cmp rax,rbx\n");
            printf("setl al\n");
            printf("movzx rax,al\n");
            break;

        case '>':
            printf("cmp rax,rbx\n");
            printf("setg al\n");
            printf("movzx rax,al\n");
            break;
        case 'L':   // <=
    	    printf("cmp rax,rbx\n");
    	    printf("setle al\n");
    	    printf("movzx rax,al\n");
    	    break;

	case 'G':   // >=
    	    printf("cmp rax,rbx\n");
    	    printf("setge al\n");
    	    printf("movzx rax,al\n");
    	    break;

	case 'E':   // ==
    	    printf("cmp rax,rbx\n");
    	    printf("sete al\n");
    	    printf("movzx rax,al\n");
    	    break;

	case 'N':   // !=
    	   printf("cmp rax,rbx\n");
    	   printf("setne al\n");
    	   printf("movzx rax,al\n");
    	   break;
    	case 'A':
           printf("cmp rax,0\n");
           printf("setne al\n");
           printf("movzx rax,al\n");

           printf("cmp rbx,0\n");
           printf("setne bl\n");
           printf("movzx rbx,bl\n");
           printf("imul rax,rbx\n");  
           break;
        case 'O':
	  printf("cmp rax,0\n");
    	  printf("setne al\n");
    	  printf("movzx rax,al\n");

    	  printf("cmp rbx,0\n");
    	  printf("setne bl\n");
    	  printf("movzx rbx,bl\n");

    	  printf("or rax,rbx\n");
	break; 
	
    	     
    }

    printf("push rax\n");

    return -1;
}

void freeTree(struct tnode *t)
{
    if(t == NULL){
        return;
    }

    freeTree(t->left);
    freeTree(t->middle);
    freeTree(t->right);
    free(t);
}
