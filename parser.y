%{
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "exprtree.h"
#include "semantic.h"
#include "symboltable.h"

extern int stringCount;
extern char stringLiterals[MAX_STRING_COUNT][MAX_STRING_LITERAL_LEN];

int yylex();
int yyerror(char *msg);
extern FILE *yyin;


struct tnode* makeStringNode(char *s);

struct tnode *programRoot = NULL;
struct tnode *functionRoot = NULL;
int outputMode = 0;
int inFunction = 0;
char currentFunctionName[MAX_NAME_LEN];
char functionParamNames[MAX_FUNCTION_PARAMS][MAX_NAME_LEN];
int functionParamCount = 0;
char functionLocalNames[256][MAX_NAME_LEN];
char functionLocalMangled[256][MAX_NAME_LEN];
int functionLocalCount = 0;

void printSymbolTable();
void dumpSymbolTable(FILE *out);
void emitAssembly(struct tnode *root);
void printPhaseOutput(void);
const char *tokenName(int token);
int semanticErrors = 0;
int inferAssignedType(struct tnode *node);
struct tnode *appendSequence(struct tnode *left, struct tnode *right);
void splitTopLevel(struct tnode *root);
void beginFunction(char *name);
void endFunction(void);
void addFunctionParam(char *name);
int isFunctionParam(char *name);
int lookupFunctionLocal(char *name, char *out);
void registerFunctionLocal(char *name, char *out);
void resolveNameForRead(char *name, char *out);
void resolveNameForWrite(char *name, char *out);
int shouldInstallResolvedName(char *resolvedName);
void copyResolvedName(char *dest, const char *src);
void buildFunctionLocalName(const char *functionName, const char *name, char *out);
%}

%union{
    struct tnode *node;
    int num;
    char name[MAX_STRING_LITERAL_LEN];
}

%token PRINT
%token PRINTLN
%token READ
%token ARRAY
%token FUNC ENDFUNC RETURN
%token <num> NUM
%token <name> ID
%token PLUS MINUS MUL DIV MOD
%token LT GT
%token IF ELSE ELSEIF ENDIF
%token WHILE ENDWHILE
%token LP RP
%token LE GE EQ NE
%token AND OR NOT
%token BREAK CONTINUE
%token FOR ENDFOR
%token <name> STRING

%type <node> E stmt stmtlist elseiflist
%type <node> assign funcdef paramlist optparams arglist optargs optbody toplist topitem optreturn

%start S
%expect 5

%left OR
%left AND
%left LT GT LE GE EQ NE
%left PLUS MINUS
%left MUL DIV MOD
%right NOT
%right UMINUS

%nonassoc IFX
%nonassoc ELSE

%%


S : toplist
    {
        splitTopLevel($1);
    }
;

toplist
    : topitem
      { $$ = $1; }
    | toplist separator topitem
      { $$ = appendSequence($1, $3); }
    | toplist separator
      { $$ = $1; }
    | separator toplist
      { $$ = $2; }
;

topitem
    : funcdef
      { $$ = $1; }
    | stmt
      { $$ = $1; }
;

funcdef
    : FUNC ID { beginFunction($2); } LP optparams RP separator optbody optreturn ENDFUNC
      { $$ = makeFuncNode($2, $5, $8, $9); endFunction(); }
;

optbody
    : stmtlist separator
      { $$ = $1; }
    |
      { $$ = NULL; }
;

optreturn
    : RETURN E separator
      { $$ = $2; }
    |
      { $$ = NULL; }
;

optparams
    : paramlist
      { $$ = $1; }
    |
      { $$ = NULL; }
;

stmtlist
    : stmt
      { $$ = $1; }
    | stmtlist separator stmt
      {
          if($1 == NULL) $$ = $3;
          else if($3 == NULL) $$ = $1;
          else $$ = makeOperatorNode('S', $1, $3);
      }
    | stmtlist separator
      {
          $$ = $1;
      }
    | separator stmtlist
      {
          $$ = $2;
      }
;

stmt
    : ARRAY ID '[' NUM ']'
      {
          char resolved[MAX_NAME_LEN];
          resolveNameForWrite($2, resolved);
          if(shouldInstallResolvedName(resolved)){
              installArray(resolved, $4);
          }
          $$ = makeArrayDeclNode(resolved, $4);
      }

    | ID '=' E
      {
          char resolved[MAX_NAME_LEN];
          resolveNameForWrite($1, resolved);
          if(shouldInstallResolvedName(resolved)){
              if(inferAssignedType($3) == 1)
                  install(resolved, TYPE_STRING);   // string
              else
                  install(resolved, TYPE_INT);   // int
          }

          $$ = makeOperatorNode('=',makeVarNode(resolved),$3);
      }

    | ID '[' E ']' '=' E
      {
          char resolved[MAX_NAME_LEN];
          resolveNameForWrite($1, resolved);
          if(shouldInstallResolvedName(resolved)){
              install(resolved, TYPE_INT_ARRAY);
          }
          $$ = makeOperatorNode('=', makeArrayNode(resolved, $3), $6);
      }
    
    
     | PRINT LP E RP 
        {
            $$ = makeOperatorNode('P',$3,NULL);
        }

     | PRINTLN LP E RP
        {
            $$ = makeOperatorNode('Z',$3,NULL);
        }
        
        
     | READ LP ID RP
        {
            char resolved[MAX_NAME_LEN];
            resolveNameForWrite($3, resolved);
            if(shouldInstallResolvedName(resolved)){
                install(resolved, TYPE_STRING);   // mark as string variable
            }
            $$ = makeReadNode(resolved);
        }

    | ID LP optargs RP
        {
            $$ = makeOperatorNode('X', makeCallNode($1, $3), NULL);
        }
     
     

    | IF LP E RP  stmtlist ENDIF  %prec IFX
        {
           $$ = makeIfNode($3,$5,NULL);
        }

    | IF LP E RP  stmtlist ELSE  stmtlist ENDIF 
        { $$=makeIfNode($3,$5,$7); }

    | IF LP E RP  stmtlist elseiflist ENDIF 
        { $$=makeIfNode($3,$5,$6); }
    | WHILE LP E RP  stmtlist ENDWHILE 
        {
         $$ = makeOperatorNode('W',$3,$5); 
        }
    | FOR LP assign ';' E ';' assign RP  stmtlist ENDFOR 
    {
      struct tnode *init = $3;
      struct tnode *cond = $5;
      struct tnode *update = $7;
      struct tnode *body = $9;

      struct tnode *body_update = makeOperatorNode('S', body, update);

      struct tnode *forNode = makeOperatorNode('F', init, cond);
	forNode->middle = body_update;

	$$ = forNode;
    }
        | BREAK 
    { $$ = makeOperatorNode('B',NULL,NULL); }

        | CONTINUE 
    { $$ = makeOperatorNode('C',NULL,NULL); }
    
    
;

elseiflist
    : ELSEIF LP E RP  stmtlist
        { $$=makeIfNode($3,$5,NULL); }

    | ELSEIF LP E RP  stmtlist elseiflist
        { $$=makeIfNode($3,$5,$6); }

    | ELSE  stmtlist
        { $$=$2; }
;
assign
    : ID '=' E
      {
      char resolved[MAX_NAME_LEN];
      resolveNameForWrite($1, resolved);
      if(shouldInstallResolvedName(resolved)){
          install(resolved, inferAssignedType($3));
      }
      $$ = makeOperatorNode('=', makeVarNode(resolved), $3); 
      }

    | ID '[' E ']' '=' E
      {
      char resolved[MAX_NAME_LEN];
      resolveNameForWrite($1, resolved);
      if(shouldInstallResolvedName(resolved)){
          install(resolved, TYPE_INT_ARRAY);
      }
      $$ = makeOperatorNode('=', makeArrayNode(resolved, $3), $6);
      }
;

E
    : E PLUS E { $$=makeOperatorNode('+',$1,$3); }
    | E MINUS E { $$=makeOperatorNode('-',$1,$3); }
    | E MUL E  { $$=makeOperatorNode('*',$1,$3); }
    | E DIV E   { $$=makeOperatorNode('/',$1,$3); }
    | E MOD E   { $$=makeOperatorNode('%',$1,$3); }
    
    | MINUS E %prec UMINUS { $$ = makeOperatorNode('U',$2,NULL); }
    
    | E AND E { $$ = makeOperatorNode('A',$1,$3); }
    | E OR E  { $$ = makeOperatorNode('O',$1,$3); }
    | NOT E   { $$ = makeOperatorNode('!',$2,NULL); }
    
    | E LT E   { $$=makeOperatorNode('<',$1,$3); }
    | E GT E   { $$=makeOperatorNode('>',$1,$3); }
    | E LE E { $$=makeOperatorNode('L',$1,$3); }
    | E GE E { $$=makeOperatorNode('G',$1,$3); }
    | E EQ E { $$=makeOperatorNode('E',$1,$3); }
    | E NE E { $$=makeOperatorNode('N',$1,$3); }
    
    | STRING { $$ = makeStringNode($1); }
    
    | LP E RP  { $$=$2; }
    | READ LP RP { $$ = makeOperatorNode('R',NULL,NULL); }
    | ID LP optargs RP { $$ = makeCallNode($1, $3); }
    | NUM      { $$=makeLeafNode($1); }
    | ID '[' E ']' {
        char resolved[MAX_NAME_LEN];
        resolveNameForRead($1, resolved);
        $$=makeArrayNode(resolved, $3);
      }
    | ID       {
        char resolved[MAX_NAME_LEN];
        resolveNameForRead($1, resolved);
        $$=makeVarNode(resolved);
      }
;

optargs
    : arglist
      { $$ = $1; }
    |
      { $$ = NULL; }
;

arglist
    : E
      { $$ = $1; }
    | arglist ',' E
      { $$ = makeOperatorNode('S', $1, $3); }
;

separator
    : '\n'
    | ';'
;

paramlist
    : ID
      { addFunctionParam($1); $$ = makeVarNode($1); }
    | paramlist ',' ID
      { addFunctionParam($3); $$ = makeOperatorNode('S', $1, makeVarNode($3)); }
;



%%

void emitAssembly(struct tnode *root){
    printf("section .data\n");
    printf("fmt db \"%%ld\",0\n");
    printf("infmt db \"%%ld\",0\n");
    printf("fmtln db \"%%ld\",10,0\n");
    printf("fmtstr db \"%%s\",0\n");
    printf("fmtstrln db \"%%s\",10,0\n");
    printf("infmtstr db \"%%s\",0\n");
    printf("oobmsg db \"Array index out of bounds\",0\n");
    printf("divmsg db \"Division by zero\",0\n");
    printf("badintmsg db \"Invalid integer input\",0\n");
    printf("badstrmsg db \"Invalid string input\",0\n");
    printf("inputbuf times 256 db 0\n");

    printSymbolTable();

    for(int i=0;i<stringCount;i++){
         printf("msg%d db %s,0\n", i, stringLiterals[i]);
    }

    printf("\nsection .text\n");
    printf("extern printf\n");
    printf("extern getchar\n");
    printf("extern strcpy\n");
    printf("extern exit\n");
    printf("global main\n");

    printf("read_line:\n");
    printf("push rbp\n");
    printf("mov rbp,rsp\n");
    printf("push rbx\n");
    printf("push r12\n");
    printf("push r13\n");
    printf("mov r12,rdi\n");
    printf("mov r13,rsi\n");
    printf("xor ebx,ebx\n");
    printf("RL_loop:\n");
    printf("sub rsp,8\n");
    printf("call getchar\n");
    printf("add rsp,8\n");
    printf("cmp eax,-1\n");
    printf("je RL_eof\n");
    printf("cmp eax,10\n");
    printf("je RL_done\n");
    printf("cmp rbx,r13\n");
    printf("jae RL_overflow\n");
    printf("mov [r12+rbx],al\n");
    printf("inc rbx\n");
    printf("jmp RL_loop\n");
    printf("RL_overflow:\n");
    printf("sub rsp,8\n");
    printf("call getchar\n");
    printf("add rsp,8\n");
    printf("cmp eax,-1\n");
    printf("je RL_overflow_done\n");
    printf("cmp eax,10\n");
    printf("jne RL_overflow\n");
    printf("RL_overflow_done:\n");
    printf("mov byte [r12+r13],0\n");
    printf("mov rax,-1\n");
    printf("jmp RL_exit\n");
    printf("RL_eof:\n");
    printf("cmp rbx,0\n");
    printf("jne RL_done\n");
    printf("mov byte [r12],0\n");
    printf("mov rax,-2\n");
    printf("jmp RL_exit\n");
    printf("RL_done:\n");
    printf("mov byte [r12+rbx],0\n");
    printf("mov rax,rbx\n");
    printf("RL_exit:\n");
    printf("pop r13\n");
    printf("pop r12\n");
    printf("pop rbx\n");
    printf("mov rsp,rbp\n");
    printf("pop rbp\n");
    printf("ret\n");

    printf("parse_int_line:\n");
    printf("push rbp\n");
    printf("mov rbp,rsp\n");
    printf("push rbx\n");
    printf("mov rsi,rdi\n");
    printf("xor eax,eax\n");
    printf("xor ecx,ecx\n");
    printf("xor ebx,ebx\n");
    printf("PI_skip_leading:\n");
    printf("movzx edx,byte [rsi]\n");
    printf("cmp dl,32\n");
    printf("je PI_advance_leading\n");
    printf("cmp dl,9\n");
    printf("je PI_advance_leading\n");
    printf("cmp dl,13\n");
    printf("je PI_advance_leading\n");
    printf("jmp PI_sign\n");
    printf("PI_advance_leading:\n");
    printf("inc rsi\n");
    printf("jmp PI_skip_leading\n");
    printf("PI_sign:\n");
    printf("cmp dl,'-'\n");
    printf("jne PI_plus\n");
    printf("mov bl,1\n");
    printf("inc rsi\n");
    printf("jmp PI_digits\n");
    printf("PI_plus:\n");
    printf("cmp dl,'+'\n");
    printf("jne PI_digits\n");
    printf("inc rsi\n");
    printf("PI_digits:\n");
    printf("movzx edx,byte [rsi]\n");
    printf("cmp dl,'0'\n");
    printf("jb PI_after_digits\n");
    printf("cmp dl,'9'\n");
    printf("ja PI_after_digits\n");
    printf("imul rax,rax,10\n");
    printf("sub dl,'0'\n");
    printf("movzx rdx,dl\n");
    printf("add rax,rdx\n");
    printf("inc rsi\n");
    printf("inc rcx\n");
    printf("jmp PI_digits\n");
    printf("PI_after_digits:\n");
    printf("cmp rcx,0\n");
    printf("je PI_fail\n");
    printf("PI_skip_trailing:\n");
    printf("movzx edx,byte [rsi]\n");
    printf("cmp dl,0\n");
    printf("je PI_done\n");
    printf("cmp dl,32\n");
    printf("je PI_advance_trailing\n");
    printf("cmp dl,9\n");
    printf("je PI_advance_trailing\n");
    printf("cmp dl,13\n");
    printf("je PI_advance_trailing\n");
    printf("jmp PI_fail\n");
    printf("PI_advance_trailing:\n");
    printf("inc rsi\n");
    printf("jmp PI_skip_trailing\n");
    printf("PI_done:\n");
    printf("cmp bl,1\n");
    printf("jne PI_success\n");
    printf("neg rax\n");
    printf("PI_success:\n");
    printf("mov rdx,1\n");
    printf("pop rbx\n");
    printf("mov rsp,rbp\n");
    printf("pop rbp\n");
    printf("ret\n");
    printf("PI_fail:\n");
    printf("xor eax,eax\n");
    printf("xor edx,edx\n");
    printf("pop rbx\n");
    printf("mov rsp,rbp\n");
    printf("pop rbp\n");
    printf("ret\n");

    printf("read_int_expr:\n");
    printf("push rbp\n");
    printf("mov rbp,rsp\n");
    printf("push rbx\n");
    printf("push r12\n");
    printf("xor ebx,ebx\n");
    printf("xor r12d,r12d\n");
    printf("RI_skip_ws:\n");
    printf("sub rsp,8\n");
    printf("call getchar\n");
    printf("add rsp,8\n");
    printf("cmp eax,-1\n");
    printf("je bad_int_input_error\n");
    printf("cmp eax,32\n");
    printf("je RI_skip_ws\n");
    printf("cmp eax,9\n");
    printf("je RI_skip_ws\n");
    printf("cmp eax,10\n");
    printf("je RI_skip_ws\n");
    printf("cmp eax,13\n");
    printf("je RI_skip_ws\n");
    printf("cmp eax,'-'\n");
    printf("jne RI_plus\n");
    printf("mov r12b,1\n");
    printf("jmp RI_first_digit\n");
    printf("RI_plus:\n");
    printf("cmp eax,'+'\n");
    printf("jne RI_check_digit\n");
    printf("RI_first_digit:\n");
    printf("sub rsp,8\n");
    printf("call getchar\n");
    printf("add rsp,8\n");
    printf("cmp eax,-1\n");
    printf("je bad_int_input_error\n");
    printf("RI_check_digit:\n");
    printf("cmp eax,'0'\n");
    printf("jb bad_int_input_error\n");
    printf("cmp eax,'9'\n");
    printf("ja bad_int_input_error\n");
    printf("RI_digits:\n");
    printf("imul rbx,rbx,10\n");
    printf("sub eax,'0'\n");
    printf("movzx rdx,al\n");
    printf("add rbx,rdx\n");
    printf("sub rsp,8\n");
    printf("call getchar\n");
    printf("add rsp,8\n");
    printf("cmp eax,-1\n");
    printf("je RI_done\n");
    printf("cmp eax,32\n");
    printf("je RI_done\n");
    printf("cmp eax,9\n");
    printf("je RI_done\n");
    printf("cmp eax,10\n");
    printf("je RI_done\n");
    printf("cmp eax,13\n");
    printf("je RI_done\n");
    printf("cmp eax,'0'\n");
    printf("jb bad_int_input_error\n");
    printf("cmp eax,'9'\n");
    printf("ja bad_int_input_error\n");
    printf("jmp RI_digits\n");
    printf("RI_done:\n");
    printf("mov rax,rbx\n");
    printf("cmp r12b,1\n");
    printf("jne RI_exit\n");
    printf("neg rax\n");
    printf("RI_exit:\n");
    printf("pop r12\n");
    printf("pop rbx\n");
    printf("mov rsp,rbp\n");
    printf("pop rbp\n");
    printf("ret\n");

    printf("read_string_into:\n");
    printf("push rbp\n");
    printf("mov rbp,rsp\n");
    printf("mov rsi,99\n");
    printf("call read_line\n");
    printf("cmp rax,-1\n");
    printf("je bad_str_input_error\n");
    printf("cmp rax,-2\n");
    printf("je bad_str_input_error\n");
    printf("mov rsp,rbp\n");
    printf("pop rbp\n");
    printf("ret\n");

    emitFunctionDefinitions(functionRoot);

    printf("main:\n");

    codeGen(root);

    printf("mov rax,0\n");
    printf("ret\n");

    // Error handlers
    printf("div_error:\n");
    printf("lea rdi,[rel divmsg]\n");
    printf("xor rax,rax\n");
    printf("call printf\n");
    printf("mov edi,1\n");
    printf("call exit\n");

    printf("oob_error:\n");
    printf("lea rdi,[rel oobmsg]\n");
    printf("xor rax,rax\n");
    printf("call printf\n");
    printf("mov edi,1\n");
    printf("call exit\n");

    printf("bad_int_input_error:\n");
    printf("lea rdi,[rel badintmsg]\n");
    printf("xor rax,rax\n");
    printf("call printf\n");
    printf("mov edi,1\n");
    printf("call exit\n");

    printf("bad_str_input_error:\n");
    printf("lea rdi,[rel badstrmsg]\n");
    printf("xor rax,rax\n");
    printf("call printf\n");
    printf("mov edi,1\n");
    printf("call exit\n");

    printf("section .note.GNU-stack noalloc noexec nowrite progbits\n");
}

void printPhaseOutput(void){
    switch(outputMode){
        case 1:
            if(functionRoot != NULL){
                printAst(functionRoot, stdout, 0);
                if(programRoot != NULL){
                    printf("Main\n");
                }
            }
            if(programRoot != NULL){
                printAst(programRoot, stdout, 0);
            }
            break;
        case 2:
            annotateSymbolScopes(functionRoot, programRoot);
            dumpSymbolTable(stdout);
            break;
        default:
            emitAssembly(programRoot);
            break;
    }
}

struct tnode *appendSequence(struct tnode *left, struct tnode *right){
    if(left == NULL) return right;
    if(right == NULL) return left;
    return makeOperatorNode('S', left, right);
}

void beginFunction(char *name){
    inFunction = 1;
    copyResolvedName(currentFunctionName, name);
    functionParamCount = 0;
    functionLocalCount = 0;
}

void endFunction(void){
    inFunction = 0;
    currentFunctionName[0] = '\0';
    functionParamCount = 0;
    functionLocalCount = 0;
}

void addFunctionParam(char *name){
    if(functionParamCount >= MAX_FUNCTION_PARAMS){
        fprintf(stderr, "Too many parameters in function '%s'\n", currentFunctionName);
        exit(1);
    }

    copyResolvedName(functionParamNames[functionParamCount], name);
    functionParamCount++;
}

int isFunctionParam(char *name){
    int i;

    for(i = 0; i < functionParamCount; i++){
        if(strcmp(functionParamNames[i], name) == 0){
            return 1;
        }
    }

    return 0;
}

int lookupFunctionLocal(char *name, char *out){
    int i;

    for(i = 0; i < functionLocalCount; i++){
        if(strcmp(functionLocalNames[i], name) == 0){
            copyResolvedName(out, functionLocalMangled[i]);
            return 1;
        }
    }

    return 0;
}

void registerFunctionLocal(char *name, char *out){
    if(lookupFunctionLocal(name, out)){
        return;
    }

    if(functionLocalCount >= 256){
        fprintf(stderr, "Too many local variables in function '%s'\n", currentFunctionName);
        exit(1);
    }

    copyResolvedName(functionLocalNames[functionLocalCount], name);
    buildFunctionLocalName(currentFunctionName, name, functionLocalMangled[functionLocalCount]);
    copyResolvedName(out, functionLocalMangled[functionLocalCount]);
    functionLocalCount++;
}

void resolveNameForRead(char *name, char *out){
    if(inFunction && lookupFunctionLocal(name, out)){
        return;
    }

    copyResolvedName(out, name);
}

void resolveNameForWrite(char *name, char *out){
    if(inFunction && !isFunctionParam(name)){
        registerFunctionLocal(name, out);
        return;
    }

    copyResolvedName(out, name);
}

int shouldInstallResolvedName(char *resolvedName){
    if(isFunctionParam(resolvedName)){
        return 0;
    }

    if(!inFunction){
        return 1;
    }

    return strcmp(resolvedName, currentFunctionName) != 0;
}

void copyResolvedName(char *dest, const char *src){
    if(strlen(src) >= MAX_NAME_LEN){
        fprintf(stderr, "Identifier too long: %s\n", src);
        exit(1);
    }

    snprintf(dest, MAX_NAME_LEN, "%s", src);
}

void buildFunctionLocalName(const char *functionName, const char *name, char *out){
    int needed = snprintf(NULL, 0, "__%s_%s", functionName, name);

    if(needed < 0 || needed >= MAX_NAME_LEN){
        fprintf(stderr, "Function-local name too long for '%s' and '%s'\n", functionName, name);
        exit(1);
    }

    snprintf(out, MAX_NAME_LEN, "__%s_%s", functionName, name);
}

static void splitTopLevelNode(struct tnode *root)
{
    if(root == NULL){
        return;
    }

    if(root->op == 'S'){
        struct tnode *left = root->left;
        struct tnode *right = root->right;
        root->left = root->middle = root->right = NULL;
        free(root);
        splitTopLevelNode(left);
        splitTopLevelNode(right);
        return;
    }

    if(root->op == 'Q'){
        functionRoot = appendSequence(functionRoot, root);
    }
    else{
        programRoot = appendSequence(programRoot, root);
    }
}

void splitTopLevel(struct tnode *root){
    functionRoot = NULL;
    programRoot = NULL;
    splitTopLevelNode(root);
}

const char *tokenName(int token){
    switch(token){
        case PRINT: return "PRINT";
        case PRINTLN: return "PRINTLN";
        case READ: return "READ";
        case ARRAY: return "ARRAY";
        case FUNC: return "FUNC";
        case ENDFUNC: return "ENDFUNC";
        case RETURN: return "RETURN";
        case NUM: return "NUM";
        case ID: return "ID";
        case PLUS: return "PLUS";
        case MINUS: return "MINUS";
        case MUL: return "MUL";
        case DIV: return "DIV";
        case MOD: return "MOD";
        case LT: return "LT";
        case GT: return "GT";
        case IF: return "IF";
        case ELSE: return "ELSE";
        case ELSEIF: return "ELSEIF";
        case ENDIF: return "ENDIF";
        case WHILE: return "WHILE";
        case ENDWHILE: return "ENDWHILE";
        case LP: return "LP";
        case RP: return "RP";
        case LE: return "LE";
        case GE: return "GE";
        case EQ: return "EQ";
        case NE: return "NE";
        case AND: return "AND";
        case OR: return "OR";
        case NOT: return "NOT";
        case BREAK: return "BREAK";
        case CONTINUE: return "CONTINUE";
        case FOR: return "FOR";
        case ENDFOR: return "ENDFOR";
        case STRING: return "STRING";
        case '\n': return "NEWLINE";
        case ';': return "SEMICOLON";
        case '=': return "ASSIGN";
        case '[': return "LBRACKET";
        case ']': return "RBRACKET";
        case ',': return "COMMA";
        default: return "UNKNOWN";
    }
}

int inferAssignedType(struct tnode *node){
    int type;

    if(node == NULL){
        return 0;
    }

    if(node->type == 3){
        return TYPE_STRING;
    }

    if(node->type == 1){
        type = getType(node->varname);
        if(type == TYPE_STRING){
            return TYPE_STRING;
        }
        if(type == TYPE_INT){
            return type;
        }
    }

    return TYPE_INT;
}

int main(int argc, char *argv[]){
    int parseStatus;
    int tokenMode = 0;
    int argIndex = 1;
    char *sourcePath = NULL;
    FILE *sourceFile = NULL;

    if(argIndex < argc){
        if(strcmp(argv[argIndex], "--tokens") == 0){
            tokenMode = 1;
            argIndex++;
        }
        else if(strcmp(argv[argIndex], "--ast") == 0){
            outputMode = 1;
            argIndex++;
        }
        else if(strcmp(argv[argIndex], "--symbols") == 0){
            outputMode = 2;
            argIndex++;
        }
        else if(strcmp(argv[argIndex], "--semantic") == 0){
            outputMode = 3;
            argIndex++;
        }
    }

    if(argIndex < argc){
        sourcePath = argv[argIndex];
        argIndex++;
    }

    if(argIndex < argc){
        fprintf(stderr, "Usage: %s [--tokens|--ast|--symbols|--semantic] [source-file]\n", argv[0]);
        return 1;
    }

    if(sourcePath != NULL){
        sourceFile = fopen(sourcePath, "r");
        if(sourceFile == NULL){
            perror(sourcePath);
            return 1;
        }
        yyin = sourceFile;
    }

    if(tokenMode){
        int token;

        while((token = yylex()) != 0){
            switch(token){
                case NUM:
                    printf("%s(%d)\n", tokenName(token), yylval.num);
                    break;
                case ID:
                case STRING:
                    printf("%s(%s)\n", tokenName(token), yylval.name);
                    break;
                default:
                    printf("%s\n", tokenName(token));
                    break;
            }
        }

        if(sourceFile != NULL){
            fclose(sourceFile);
        }
        return 0;
    }

    parseStatus = yyparse();
    if(parseStatus != 0 || (programRoot == NULL && functionRoot == NULL)){
        freeTree(functionRoot);
        freeTree(programRoot);
        freeSymbolTable();
        if(sourceFile != NULL){
            fclose(sourceFile);
        }
        return 1;
    }

    semanticErrors = analyzeSemantics(functionRoot, programRoot, NULL);

    if(semanticErrors != 0){
        freeTree(functionRoot);
        freeTree(programRoot);
        freeSymbolTable();
        if(sourceFile != NULL){
            fclose(sourceFile);
        }
        return 1;
    }

    if(outputMode == 3){
        analyzeSemantics(functionRoot, programRoot, stdout);
        freeTree(functionRoot);
        freeTree(programRoot);
        freeSymbolTable();
        if(sourceFile != NULL){
            fclose(sourceFile);
        }
        return 0;
    }

    printPhaseOutput();
    freeTree(functionRoot);
    freeTree(programRoot);
    freeSymbolTable();
    if(sourceFile != NULL){
        fclose(sourceFile);
    }
    return 0;
}

int yyerror(char *msg){
    fprintf(stderr,"Invalid Expression\n");
    return 1;
}
