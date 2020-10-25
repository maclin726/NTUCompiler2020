%option noyywrap 
%{
#include <stdio.h>
#include "header.h" 
int linenumber;
symtab * lookup();
symtab * ptr;
void     insertID();
void 	 printSymTab();
/* You need to define for all tokens in C--, here are some examples */
#define INT			10
#define FLOAT 		11
#define STRING		12
#define COMMENT		13
#define IDENTIFIER	14

#define OP_PLUS		20
#define OP_MINUS	21
#define OP_MUL		22
#define OP_DIV		23

#define OP_GE		30
#define OP_LE		31
#define OP_GT		32
#define OP_LT		33
#define	OP_NE		34
#define OP_EQ		35


#define OP_ASSIGN 	40
#define OP_AND		41
#define OP_OR		42
#define OP_NOT 		43

#define MK_LPAREN 	50 
#define MK_RPAREN 	51
#define MK_LBRACK	52
#define MK_RBRACK	53
#define MK_LBRACE	54
#define MK_RBRACE	55

#define RW_RETURN	60
#define RW_TYPEDEF	61
#define RW_IF		62
#define RW_ELSE		63
#define RW_INT		64
#define RW_FLOAT	65
#define RW_FOR		66
#define RW_VOID		67
#define RW_WHILE	68

#define ERROR 		100 

%}

letter   [A-Za-z]
digit    [0-9]
ID	 {letter}({letter}|{digit}|"_")*
WS	 [ \t]+

/* Int, Float, String, comment */

Int_constant {digit}+
Float_constant ({digit}+(\.{digit}+)?("e"[+-]?{digit}+)?)|({digit}+)?(\.{digit}+)("e"[+-]?{digit}+)?
String_constant ["]([^"\\\n]|\\.|\\\n)*["]
Comment \/\*(\*\**[^\*\/]|[^\*])*\*\**\/

OP_plus "+"
OP_minus "-"
OP_mul "*"
OP_div "/"

OP_ge ">="
OP_le "<="
OP_gt ">"
OP_lt "<"
OP_ne "!="
OP_eq "=="

OP_assign "="
OP_and "&&"
OP_or "||"
OP_not "!"

/* Other operators appear here */ 


newline  "\n"

MK_lparen "("
MK_rparen ")"
MK_lbrack "["
MK_rbrack "]"
MK_lbrace "{"
MK_rbrace "}"
MK_comma  ","
MK_semicol ";"
MK_dot  "."

/* Reserved words */
RW_return "return"
RW_typedef "typedef"
RW_if "if"
RW_else "else"
RW_int "int"
RW_float "float"
RW_for "for"
RW_void "void"
RW_while "while"

/* Other separators appear here */

error    .

%%

{RW_return}  	{ /*return RW_RETURN;*/ }
{RW_typedef} 	{ /*return RW_TYPEDEF;*/ }
{RW_if} 		{ /*return RW_IF;*/ }
{RW_else} 		{ /*return RW_ELSE;*/ }
{RW_int} 		{ /*return RW_INT;*/ }
{RW_float} 		{ /*return RW_FLOAT;*/ }
{RW_for} 		{ /*return RW_FOR;*/ }
{RW_void} 		{ /*return RW_VOID;*/ }
{RW_while} 		{ /*return RW_WHILE;*/ }

{WS}            		{ }
{Int_constant}         	{ /*return INT;*/ }
{Float_constant}       	{ /*return FLOAT;*/ }
{String_constant}		{ /*return STRING;*/ }
{Comment}				{ printf("%s\n", yytext); }
{ID}	{ 
			ptr = lookup(yytext);
			if (ptr == NULL)
			     insertID(yytext);	
			else 
			     ptr->counter++;
			/*return IDENTIFIER*/;
		}

{OP_plus} { /*return OP_PLUS;*/ }
{OP_minus} { /*return OP_MINUS;*/ }
{OP_mul} { /*return OP_MUL;*/ }
{OP_div} { /*return OP_DIV;*/ }

{OP_ge} { /*return OP_GE;*/ }
{OP_le} { /*return OP_LE;*/ }
{OP_gt} { /*return OP_GT;*/ }
{OP_lt} { /*return OP_LT;*/ }
{OP_ne} { /*return OP_NE;*/ }
{OP_eq} { /*return OP_EQ;*/ }

{OP_assign}     { /*return OP_ASSIGN;*/ }
{OP_and}		{ /*return OP_AND;*/ }
{OP_not}		{ /*return OP_NOT;*/ }
{OP_or}         { /*return OP_OR;*/ }
{MK_lparen}     { /*return MK_LPAREN;*/ }
{MK_rparen}     { /*return MK_RPAREN;*/ }
{MK_lbrack}     { /*return MK_LBRACK;*/ }
{MK_rbrack}     { /*return MK_RBRACK;*/ }
{MK_lbrace}     { /*return MK_LBRACE;*/ }
{MK_rbrace}     { /*return MK_RBRACE;*/ }
{MK_comma}      {}
{MK_semicol}    {}
{newline}       { linenumber += 1; }
{error}         { printf("ERROR: invalid token [%s]\n", yytext); exit(1); }


%%

int main(int argc, char **argv)
{
 argc--; ++argv;
   if (argc > 0)
      yyin = fopen(argv[0], "r");
   else
      yyin = stdin;
   yylex();
   printSymTab();
}

