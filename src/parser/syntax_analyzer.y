%{
#include <stdio.h>
#include <stdlib.h>    
#include <string.h>
#include <stdarg.h>

#include "syntax_tree.h"

// external functions from lex
extern int yylex();
extern int yyparse();
extern int yyrestart();
extern FILE * yyin;

// external variables from lexical_analyzer module
extern int lines;
extern char * yytext;
extern int pos_end;
extern int pos_start;

// Global syntax tree
syntax_tree *gt;

// Error reporting
void yyerror(const char *s);

// Helper functions written for you with love
syntax_tree_node *node(const char *node_name, int children_num, ...);
%}

/* TODO: Complete this definition.
   Hint: See pass_node(), node(), and syntax_tree.h.
         Use forward declaring. */
%union {
    void* node;
    char* string;
}

/* TODO: Your tokens here. */

/* Keyword token */
%token <string> KEYWORDELSE 
%token <string> KEYWORDIF
%token <string> KEYWORDINT
%token <string> KEYWORDRETURN
%token <string> KEYWORDVOID
%token <string> KEYWORDWHILE
%token <string> KEYWORDFLOAT
/* OP token */ 
%token <string> ADD
%token <string> MINUS
%token <string> MULTIPLY
%token <string> DIVIDE
/* RELOP token */
%token <string> LT
%token <string> ELT
%token <string> GT
%token <string> EGT
%token <string> EQ
%token <string> NEQ
%token <string> ASSIGNMENT
/* PUNCTUATION MARK token */
%token <string> SEMICOLON
%token <string> COMMA
%token <string> LPARENTHESES
%token <string> RPARENTHESES
%token <string> LBRACKETS
%token <string> RBRACKETS
%token <string> LBRACES
%token <string> RBRACES
/* ID INTEGER FLOAT token */
%token <string> ID
%token <string> INTEGER
%token <string> FLOATPOINT
/* ERROR token */
%token <string> ERROR 

/* syntax type */
/* keyword op relop id */
%type  <node> type-specifier
%type  <node> relop
%type  <node> addop
%type  <node> mulop
/* declaration */
%type  <node> declaration-list
%type  <node> declaration 
%type  <node> var-declaration
%type  <node> fun-declaration
%type  <node> local-declarations
/* statement */
%type  <node> compound-stmt
%type  <node> statement-list
%type  <node> statement
%type  <node> expression-stmt
%type  <node> iteration-stmt
%type  <node> selection-stmt
%type  <node> return-stmt
/* expression*/
%type  <node> expression
%type  <node> simple-expression
%type  <node> var
%type  <node> additive-expression
%type  <node> term
%type  <node> factor
%type  <node> integer
%type  <node> float
%type  <node> call
/* others */
%type  <node> params
%type  <node> param-list
%type  <node> param
%type  <node> args
%type  <node> arg-list
/* start symbol */
%type  <node> program

%start program

%%
/* TODO: Your rules here. */

/* Example:
program: declaration-list {$$ = node( "program", 1, $1); gt->root = $$;}
       ;
*/

program: declaration-list {$$ = node("program", 1, $1); gt->root = $$;}
       ;

declaration-list: declaration-list declaration {$$ = node("declaration-list", 2, $1, $2);}
                | declaration {$$ = node("declaration-list", 1, $1);}
                ;

declaration: var-declaration {$$ = node("declaration", 1, $1);}
           | fun-declaration {$$ = node("declaration", 1, $1);}
           ;

var-declaration: type-specifier ID SEMICOLON {$$ = node("var-declaration", 3, $1, $2, $3);}
               | type-specifier ID LBRACKETS INTEGER RBRACKETS SEMICOLON {$$ = node("var-declaration", 6, $1, $2, $3, $4, $5, $6);}
               ;

type-specifier: KEYWORDINT {$$ = node("type-specifier", 1, $1);}
              | KEYWORDFLOAT {$$ = node("type-specifier", 1, $1);}
              | KEYWORDVOID {$$ = node("type-specifier", 1, $1);}
              ;

fun-declaration: type-specifier ID LPARENTHESES params RPARENTHESES compound-stmt {$$ = node("fun-declaration", 6, $1, $2, $3, $4, $5, $6);}
               ;

params: param-list {$$ = node("params", 1, $1);}
      | KEYWORDVOID {$$ = node("params", 1, $1);}
      ;

param-list: param-list COMMA param {$$ = node("param-list", 3, $1, $2, $3);}
          | param {$$ = node("param-list", 1, $1);}
          ;

param: type-specifier ID {$$ = node("param", 2, $1, $2);}
     | type-specifier ID LBRACKETS RBRACKETS {$$ = node("param", 4, $1, $2, $3, $4);}
     ;

compound-stmt: LBRACES local-declarations statement-list RBRACES {$$ = node("compound-stmt", 4, $1, $2, $3, $4);}
             ;

local-declarations: local-declarations var-declaration {$$ = node("local-declarations", 2, $1, $2);}
                  | {$$ = node("local-declarations", 0);}
                  ;

statement-list: statement-list statement {$$ = node("statement-list", 2, $1,$2);}
              | {$$ = node("statement-list", 0);}
              ;

statement: expression-stmt {$$ = node("statement", 1, $1);}
         | compound-stmt {$$ = node("statement", 1, $1);}
         | selection-stmt {$$ = node("statement", 1, $1);}
         | iteration-stmt {$$ = node("statement", 1, $1);}
         | return-stmt {$$ = node("statement", 1, $1);}
         ;

expression-stmt: expression SEMICOLON {$$ = node("expression-stmt", 2, $1, $2);}
               | SEMICOLON {$$ = node("expression-stmt", 1, $1);}
               ;
            
selection-stmt: KEYWORDIF LPARENTHESES expression RPARENTHESES statement {$$ = node("selection-stmt", 5, $1, $2, $3, $4, $5);}
              | KEYWORDIF LPARENTHESES expression RPARENTHESES statement KEYWORDELSE statement {$$ = node("selection-stmt", 7, $1, $2, $3, $4, $5, $6, $7);}
              ;

iteration-stmt: KEYWORDWHILE LPARENTHESES expression RPARENTHESES statement {$$ = node("iteration-stmt", 5, $1, $2, $3, $4, $5);}
              ;

return-stmt: KEYWORDRETURN SEMICOLON {$$ = node("return-stmt", 2, $1, $2);}
           | KEYWORDRETURN expression SEMICOLON {$$ = node("return-stmt", 3, $1, $2, $3);}
           ;

expression: var ASSIGNMENT expression {$$ = node("expression", 3, $1, $2, $3);}
          | simple-expression {$$ = node("expression", 1, $1);}
          ;

var: ID {$$ = node("var", 1, $1);}
   | ID LBRACKETS expression RBRACKETS {$$ = node("var", 4, $1, $2, $3, $4);}
   ;

simple-expression: additive-expression relop additive-expression {$$ = node("simple-expression", 3, $1, $2, $3);}
                 | additive-expression {$$ = node("simple-expression", 1, $1);}
                 ;
relop: ELT {$$ = node("relop", 1, $1);}
     | LT {$$ = node("relop", 1, $1);}
     | GT {$$ = node("relop", 1, $1);}
     | EGT {$$ = node("relop", 1, $1);}
     | EQ {$$ = node("relop", 1, $1);}
     | NEQ {$$ = node("relop", 1, $1);}
     ;

additive-expression: additive-expression addop term {$$ = node("additive-expression", 3, $1, $2, $3);}
                   | term {$$ = node("additive-expression", 1, $1);}
                   ;

addop: ADD {$$ = node("addop", 1, $1);}
     | MINUS {$$ = node("addop", 1, $1);}
     ;

term: term mulop factor {$$ = node("term", 3, $1, $2, $3);}
    | factor {$$ = node("term", 1, $1);}
    ;

mulop: MULTIPLY {$$ = node("mulop", 1, $1);}
     | DIVIDE {$$ = node("mulop", 1, $1);}
     ;

factor: LPARENTHESES expression RPARENTHESES  {$$ = node("factor", 3, $1, $2, $3);}
      | var {$$ = node("factor", 1, $1);}
      | call {$$ = node("factor", 1, $1);}
      | integer {$$ = node("factor", 1, $1);}
      | float {$$ = node("factor", 1, $1);}
      ;

integer: INTEGER {$$ = node("integer", 1, $1);}
       ;

float: FLOATPOINT {$$ = node("float", 1, $1);}
     ;

call: ID LPARENTHESES args RPARENTHESES {$$ = node("call", 4, $1, $2, $3, $4);}
    ;

args: arg-list {$$ = node("args", 1, $1);}
    | {$$ = node("args", 0);}
    ;

arg-list: arg-list COMMA expression {$$ = node("arg-list", 3, $1, $2, $3);} 
        | expression {$$ = node("arg-list", 1, $1);}
        ;


%%

/// The error reporting function.
void yyerror(const char * s)
{
    // TO STUDENTS: This is just an example.
    // You can customize it as you like.
    fprintf(stderr, "error at line %d column %d: %s\n", lines, pos_start, s);
}

/// Parse input from file `input_path`, and prints the parsing results
/// to stdout.  If input_path is NULL, read from stdin.
///
/// This function initializes essential states before running yyparse().
syntax_tree *parse(const char *input_path)
{
    if (input_path != NULL) {
        if (!(yyin = fopen(input_path, "r"))) {
            fprintf(stderr, "[ERR] Open input file %s failed.\n", input_path);
            exit(1);
        }
    } else {
        yyin = stdin;
    }

    lines = pos_start = pos_end = 1;
    gt = new_syntax_tree();
    yyrestart(yyin);
    yyparse();
    return gt;
}

/// A helper function to quickly construct a tree node.
///
/// e.g. $$ = node("program", 1, $1);
syntax_tree_node *node(const char *name, int children_num, ...)
{
    syntax_tree_node *p = new_syntax_tree_node(name);
    syntax_tree_node *child;
    if (children_num == 0) {
        child = new_syntax_tree_node("epsilon");
        syntax_tree_add_child(p, child);
    } else {
        va_list ap;
        va_start(ap, children_num);
        for (int i = 0; i < children_num; ++i) {
            child = va_arg(ap, syntax_tree_node *);
            syntax_tree_add_child(p, child);
        }
        va_end(ap);
    }
    return p;
}
