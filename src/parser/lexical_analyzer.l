%option noyywrap

/* pre pattern definition */
letter [a-zA-Z]
digit  [0-9]

%{
/*****************声明和选项设置  begin*****************/
#include <stdio.h>
#include <stdlib.h>

#include "syntax_tree.h"
#include "syntax_analyzer.h"

int lines;
int pos_start;
int pos_end;

void pass_node(char *text){
     yylval.node = new_syntax_tree_node(text);
}

/*****************声明和选项设置  end*****************/
%}


%%
 /* to do for students */
 /* two cases for you, pass_node will send flex's token to bison */
 
 /* new line*/
"\n"      {pos_start = 0, pos_end = pos_start, lines++;}
"\r"      {pos_start = 0, pos_end = pos_start, lines++;} 

 /*whitspace*/
"\t"      {pos_start = pos_end, pos_end += 4;}
" "       {pos_start = pos_end, pos_end += 1;}

 /*keywords */
"else"    {pos_start = pos_end; pos_end += 4; pass_node(yytext); return KEYWORDELSE;}
"if"      {pos_start = pos_end; pos_end += 2; pass_node(yytext); return KEYWORDIF;}
"int"     {pos_start = pos_end; pos_end += 3; pass_node(yytext); return KEYWORDINT;}
"return"  {pos_start = pos_end; pos_end += 6; pass_node(yytext); return KEYWORDRETURN;}
"void"    {pos_start = pos_end; pos_end += 4; pass_node(yytext); return KEYWORDVOID;}
"while"   {pos_start = pos_end; pos_end += 5; pass_node(yytext); return KEYWORDWHILE;}
"float"   {pos_start = pos_end; pos_end += 5; pass_node(yytext); return KEYWORDFLOAT;}

 /* OP */
"+"       {pos_start = pos_end; pos_end += 1; pass_node(yytext); return ADD;}
"-"       {pos_start = pos_end; pos_end += 1; pass_node(yytext); return MINUS;}
"*"       {pos_start = pos_end; pos_end += 1; pass_node(yytext); return MULTIPLY;}
"/"       {pos_start = pos_end; pos_end += 1; pass_node(yytext); return DIVIDE;}

 /* RELOP */
"<"       {pos_start = pos_end; pos_end += 1; pass_node(yytext); return LT;}
"<="      {pos_start = pos_end; pos_end += 2; pass_node(yytext); return ELT;}
">"       {pos_start = pos_end; pos_end += 1; pass_node(yytext); return GT;}
">="      {pos_start = pos_end; pos_end += 1; pass_node(yytext); return EGT;}
"=="      {pos_start = pos_end; pos_end += 1; pass_node(yytext); return EQ;}
"!="      {pos_start = pos_end; pos_end += 2; pass_node(yytext); return NEQ;}
"="       {pos_start = pos_end; pos_end += 2; pass_node(yytext); return ASSIGNMENT;}

 /* PUNCTUATION MARK*/
";"       {pos_start = pos_end; pos_end += 1; pass_node(yytext); return SEMICOLON;}
","       {pos_start = pos_end; pos_end += 1; pass_node(yytext); return COMMA;}
"("       {pos_start = pos_end; pos_end += 1; pass_node(yytext); return LPARENTHESES;}
")"       {pos_start = pos_end; pos_end += 1; pass_node(yytext); return RPARENTHESES;}
"["       {pos_start = pos_end; pos_end += 1; pass_node(yytext); return LBRACKETS;}
"]"       {pos_start = pos_end; pos_end += 1; pass_node(yytext); return RBRACKETS;}
"{"       {pos_start = pos_end; pos_end += 1; pass_node(yytext); return LBRACES;}
"}"       {pos_start = pos_end; pos_end += 1; pass_node(yytext); return RBRACES;}

 /* COMMENT */
\/\*[^\*]*\*+([^*/][^*]*\*+)*\/ {
     //update lines and pos data
     for(int i = 0; i < yyleng; i++){
          if(*(yytext+i) == '\n'){
               lines += 1;
               pos_start = 0;
               pos_end = 0;
          }
          else 
               pos_end += 1;
     }
     //do nothing else}

 /* ID INTEGER FLOAT */
{letter}+   {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return ID;}}
{digit}+    {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return INTEGER;}
{digit}+\.|{digit}*\.{digit}+ {pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return FLOATPOINT;}

 /* OTHER */
.           {pos_start = pos_end; pos_end++; return ERROR;}
 
 /****请在此补全所有flex的模式与动作  end******/
%%
