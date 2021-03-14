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

static int null_flag;
static std::string cur_string;
static int lp_star_count;

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

QUOTE	          \"
NULL	          \0
ESCAPE          \\

TWO_HYPHEN      --
LP_STAR         \(\*
STAR_RP         \*\)

ALL             .

%x STRING
%x ONE_LINE_COMMENT
%x MUT_LINE_COMMENT

%%

 /* 
 * Singe Line Comments
 */

{TWO_HYPHEN}  { BEGIN(ONE_LINE_COMMENT); }
<ONE_LINE_COMMENT>{NEWLINE} {
  BEGIN(INITIAL);
  curr_lineno++;
}
<ONE_LINE_COMMENT>{ALL} {}

 /* 
 * Multi Line Comments
 */
{LP_STAR} {
  BEGIN(MUT_LINE_COMMENT);
  lp_star_count++;
}
{STAR_RP} {
  yylval.error_msg = "Unmatched *)";
  return ERROR;
}
<MUT_LINE_COMMENT>{LP_STAR} {
  lp_star_count++;
}
<MUT_LINE_COMMENT>{STAR_RP} {
  lp_star_count--;
  if(lp_star_count == 0) {
    BEGIN(INITIAL);
  }
}
<MUT_LINE_COMMENT>{NEWLINE} {
  curr_lineno++;
}
<MUT_LINE_COMMENT><<EOF>> {
  BEGIN(INITIAL);
	cool_yylval.error_msg = "EOF in comment";
	return ERROR;
}
<MUT_LINE_COMMENT>{ALL} {}

 /* 
 * Keywords. Except True & False, case insensitive
 */

{CLASS}			  { return CLASS; }
{ELSE}			  { return ELSE;  }
{FI}			  	{ return FI; }
{IF}				  { return IF; }
{IN}			  	{ return IN; }
{INHERITS}  	{ return INHERITS; }
{LET}			  	{ return LET; }
{LOOP}		  	{ return LOOP; }
{POOL}		  	{ return POOL; }
{THEN}		  	{ return THEN; }
{WHILE}		  	{ return WHILE; }
{CASE}		  	{ return CASE; }
{ESAC}		  	{ return ESAC; }
{OF}			  	{ return OF; }
{NEW}			  	{ return NEW; }
{ISVOID}	  	{ return ISVOID; }
{NOT}				  { return NOT; }

 /* 
 * Whitespace, Integers, Identifiers, and Special Notation.
 */

{WHITESPACE}  {}
{DARROW}		  { return DARROW; }
{LE}				  { return LE; }
{ASSIGN}		  { return ASSIGN; }
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
	return TYPEID;
}
{OBJECTID} {
	cool_yylval.symbol = idtable.add_string(yytext);
	return OBJECTID;
}

 /* 
 * String. 
 */

{QUOTE}	{
  BEGIN(STRING);
  cur_string = "";
  null_flag = 0;
}

<STRING>{QUOTE} {
  BEGIN(INITIAL);
  if(null_flag == 1) {
    cool_yylval.error_msg = "String contains invalid character";
    return ERROR;
  }
  else if(cur_string.size() >= MAX_STR_CONST) {
    cool_yylval.error_msg = "String constant too long";
    return ERROR;
  }
  else {
    cool_yylval.symbol = stringtable.add_string(cur_string.c_str());
    return STR_CONST;
  }
}
<STRING><<EOF>> {
  BEGIN(INITIAL);
  cool_yylval.error_msg = "EOF in string constant";
  return ERROR;
}
<STRING>{NEWLINE} {
	BEGIN(INITIAL);
	curr_lineno++;
  cool_yylval.error_msg = "Unterminated string constant";
  return ERROR;
}
<STRING>{ESCAPE}{NEWLINE} {
	curr_lineno++;
  cur_string += '\n';
}
<STRING>{NULL} {
  null_flag = 1;
}
<STRING>{ESCAPE}{ALL} {
  switch(yytext[1]) {
    case 'b':
      cur_string += '\b';
      break;
    case 't':
      cur_string += '\t';
      break;
    case 'n':
      cur_string += '\n';
      break;
    case 'f':
      cur_string += '\f';
      break;
    case '\0':
      null_flag = 1;
      break;
    default:
      cur_string += yytext[1];
      break;
  }
}
<STRING>{ALL} {
  cur_string += yytext;
}

 /* 
 * Error. 
 */

{ALL} {
    cool_yylval.error_msg = yytext;
    return ERROR;
}

%%
