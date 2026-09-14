%{
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "exprtree.h"
#include "parser.tab.h"

#define RETURN_TOKEN(tok) do { lastTokenWasNewline = 0; return (tok); } while(0)

static int lastTokenWasNewline = 0;

static void copyTokenText(char *dest, size_t size, const char *src, const char *kind)
{
    if(strlen(src) >= size){
        fprintf(stderr, "%s too long: %s\n", kind, src);
        exit(1);
    }

    snprintf(dest, size, "%s", src);
}
%}

%%

"println" { RETURN_TOKEN(PRINTLN); }
"print" { RETURN_TOKEN(PRINT); }
"read" { RETURN_TOKEN(READ); }
"array" { RETURN_TOKEN(ARRAY); }
"func" { RETURN_TOKEN(FUNC); }
"return" { RETURN_TOKEN(RETURN); }
"endfunc" { RETURN_TOKEN(ENDFUNC); }
"if"      { RETURN_TOKEN(IF); }
"elseif"  { RETURN_TOKEN(ELSEIF); }
"else"    { RETURN_TOKEN(ELSE); }
"endif"   { RETURN_TOKEN(ENDIF); }


\"[^\"]*\" {
    copyTokenText(yylval.name, sizeof(yylval.name), yytext, "String literal");
    RETURN_TOKEN(STRING);
}


"while"	  { RETURN_TOKEN(WHILE); }
"endwhile" { RETURN_TOKEN(ENDWHILE); }

"for" { RETURN_TOKEN(FOR); }
"endfor" { RETURN_TOKEN(ENDFOR); }

"and" { RETURN_TOKEN(AND); }
"or"  { RETURN_TOKEN(OR); }
"not" { RETURN_TOKEN(NOT); }

"break" { RETURN_TOKEN(BREAK); }
"continue" { RETURN_TOKEN(CONTINUE); }

[0-9]+    { yylval.num=atoi(yytext); RETURN_TOKEN(NUM); }


[a-zA-Z_][a-zA-Z0-9_]* {
    copyTokenText(yylval.name, sizeof(yylval.name), yytext, "Identifier");
    RETURN_TOKEN(ID);
}

"+" { RETURN_TOKEN(PLUS); }
"-" { RETURN_TOKEN(MINUS); }
"*" { RETURN_TOKEN(MUL); }
"/" { RETURN_TOKEN(DIV); }
"%" { RETURN_TOKEN(MOD); }

"==" { RETURN_TOKEN(EQ); }
"!=" { RETURN_TOKEN(NE); }
"<=" { RETURN_TOKEN(LE); }
">=" { RETURN_TOKEN(GE); }
"<" { RETURN_TOKEN(LT); }
">" { RETURN_TOKEN(GT); }


"(" { RETURN_TOKEN(LP); }
")" { RETURN_TOKEN(RP); }
";" { RETURN_TOKEN(';'); }

\n {
    if(lastTokenWasNewline){
        /* Collapse blank lines into a single separator token. */
    }
    else{
        lastTokenWasNewline = 1;
        return '\n';
    }
}

[ \t]+ ;

. {
    lastTokenWasNewline = 0;
    return yytext[0];
}

%%

int yywrap(){ return 1; }
