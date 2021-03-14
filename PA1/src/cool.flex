%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1025
#define YY_NO_UNPUT   /* keep g++ happy */

extern FILE *fin; /* we read from this file */

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
  if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
    YY_FATAL_ERROR( "read() in flex scanner failed");

char string_buf[MAX_STR_CONST]; /* to assemble string constants */
char *string_buf_ptr;

extern int curr_lineno;

extern YYSTYPE cool_yylval;

/*
 *  Add Your own definitions here
 */

%}

%option noyywrap

/*
 * Define names for regular expressions here.
 */

CLASS						?i:class
ELSE						?i:else
FI							?i:fi
IF							?i:if
IN							?i:in
INHERITS				?i:inherits
LET							?i:let
LOOP						?i:loop
POOL						?i:pool
THEN						?i:then
WHILE						?i:while
CASE						?i:case
ESAC						?i:esac
OF							?i:of
NEW							?i:new
ISVOID					?i:isvoid
NOT							?i:not

WHITESPACE      [ \t\r\f\v]+
DARROW          =>
LE							<=
ASSIGN					<-
NEWLINE					\n
ONE_SYMBOL	    [+/\-*=<.~,:;(){}@]

INT             [0-9]+
TRUE            t(?i:rue)
FALSE	          f(?i:alse)
TYPEID					[A-Z][a-zA-Z0-9_]*
OBJECTID				[a-z][a-zA-Z0-9_]*

ALL             .

%%

 /* 
 * Comments
 */

 /* 
 * Keywords. Except True & False, case insensitive
 */

{CLASS}			  { return (CLASS); }
{ELSE}			  { return (ELSE);  }
{FI}			  	{ return (FI); }
{IF}				  { return (IF); }
{IN}			  	{ return (IN); }
{INHERITS}  	{ return (INHERITS); }
{LET}			  	{ return (LET); }
{LOOP}		  	{ return (LOOP); }
{POOL}		  	{ return (POOL); }
{THEN}		  	{ return (THEN); }
{WHILE}		  	{ return (WHILE); }
{CASE}		  	{ return (CASE); }
{ESAC}		  	{ return (ESAC); }
{OF}			  	{ return (OF); }
{NEW}			  	{ return (NEW); }
{ISVOID}	  	{ return (ISVOID); }
{NOT}				  { return (NOT); }

 /* 
 * Whitespace, Integers, Identifiers, and Special Notation.
 */

{WHITESPACE}  {}
{DARROW}		  { return (DARROW); }
{LE}				  { return (LE); }
{ASSIGN}		  { return (ASSIGN); }
{NEWLINE}		  { curr_lineno++; }
{ONE_SYMBOL} 	{ return int(yytext[0]); }

{INT} {
	cool_yylval.symbol = inttable.add_string(yytext);
	return INT_CONST;
}
{TRUE}	{
  cool_yylval.boolean = 1;
  return BOOL_CONST;
}
{FALSE} {
  cool_yylval.boolean = 0;
  return BOOL_CONST;
}
{TYPEID} {
	cool_yylval.symbol = idtable.add_string(yytext);
	return (TYPEID);
}
{OBJECTID} {
	cool_yylval.symbol = idtable.add_string(yytext);
	return (OBJECTID);
}

 /* 
 * String. 
 */



 /* 
 * Error. 
 */

{ALL} {
    cool_yylval.error_msg = yytext;
    return ERROR;
}

%%
