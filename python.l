%{
	#include <stdlib.h>
	#include "python.tab.h"
	extern YYSTYPE yylval;
	extern int yy_val;
	extern char * yy_str;
	extern int yy_str_len;
	extern FILE * outscript;
	int i, spaces;
	#define INDENT_STACKSIZE	100
	int indent_stack[INDENT_STACKSIZE];
	int indent_stack_i = -1;
	int brack_stack = 0;
	int dedents;
	void push_indent(int);
	void pop_back_indent(int);
	char * longcomment = NULL;
	#define TAB_SPACES	8
	int yydebug = 1;
%}
%Start NEWL DEDENTING INBRACK GIANTCOMMENT
%%
<NEWL><<EOF>>	{ 
			if(indent_stack[indent_stack_i] != 0)
			{
				pop_back_indent(0);	
				printf("dedents@: %d\n", dedents);
				if(dedents--)
				{
					printf("DEDENT ");
					//may need to return more than one DEDENT here, how?!? FIXME
					//otherwise make sure plenty of newlines at end of python script
					return DEDENT;
				}
			}
			BEGIN 0;
		}
\"\"\"[^(\"\"\")]*\"\"\" { 
			printf("%s", yytext); 
			//BEGIN GIANTCOMMENT; 
			/* Presumably yytext is statically allocated?? */
			if(!longcomment)
				free(longcomment);
			longcomment = calloc(1, yyleng + 6);
			sprintf(longcomment, "=begin");
			strncpy(&(longcomment[6]), &(yytext[3]), yyleng - 6); 
			sprintf(&(longcomment[yyleng]), "\n=end\n");
			yy_str = longcomment;
			yy_str_len = yyleng + 6;
			return COMMENT;
		}
<NEWL>[ \t]*\"\"\"      { yyless(yyleng - 3); }
<NEWL>[ \t]*#![^\n]*\n	{ return NEWLINE; } 
<NEWL>[ \t]*#[^\n]*	{ 
			printf("COMMENT\n"); 
			yy_str = yytext;
			yy_str_len = yyleng;
			return COMMENT;
		}
[ \t]*#![^\n]*\n {	BEGIN NEWL; return NEWLINE; }
[ \t]*#[^\n]*	{ 
			printf("COMMENT\n");
			yy_str = yytext;
			yy_str_len = yyleng;
			return COMMENT;
		}
<DEDENTING>\n.*	{
			if(yytext[0] == ' ')
				printf("WTF2 ");
			printf("*%s\n", yytext);
			yyless(0);
			printf("dedents@: %d\n", dedents);
			if(dedents--)
			{
				printf("DEDENT ");
				return DEDENT;
			}
			BEGIN 0;
			printf("exiting dedent\n");
		}
<NEWL>\n	{ printf("nn\n"); return NEWLINE; }
\\\n		{ printf("SLASHSLASH\n"); /*yymore??*/ }
<INBRACK>\n	{}
<NEWL>[ \t]+ 	{
			spaces = 0;
			BEGIN 0;
			for(i = 0; i < yyleng; i++)
			{
				if(yytext[i] == ' ')
					spaces++;
				else if(yytext[i] ==  '\t')
					spaces += TAB_SPACES;
				//else    error
			}
			printf("spaces %d %d\n", spaces, yyleng);
			if(spaces > indent_stack[indent_stack_i])
			{
				push_indent(spaces);
				printf("\nINDENT ");
				return INDENT;
			}
			else if(spaces < indent_stack[indent_stack_i])
			{
				pop_back_indent(spaces);	
				dedents--;
				return DEDENT;
			}
		}
<NEWL>[^ \t]+	{ 
			printf("#%s\n", yytext);
			yyless(0);
			BEGIN 0; 
			printf("Turning off NEWL \n"); 
			if(indent_stack[indent_stack_i] != 0)
			{
				pop_back_indent(0);	
				if(dedents--)
				{
					printf("DEDENT ");
					return DEDENT;
				}
			}
		}
\n		{ printf("NEWLINE\n"); BEGIN NEWL; return NEWLINE; }
\:		{ printf("COLON "); return COLON; }	
\;		{ printf("SEMICOLON "); return SEMICOLON; }
\,		{ printf("COMMA " ); return COMMA; }
\.		{ return PERIOD; }
@		{ return ATSYM; }
if		{ printf("IF "); return IF; }	
elif		{ printf("ELSEIF "); return ELSEIF; }
else		{ printf("ELSE "); return ELSE; }
for		{ printf("FOR "); return FOR; }
while		{ return WHILE; }
try		{ return TRY; }
\|=		{ return OREQUAL; }
\|		{ return OR; }
\^=		{ return XOREQUAL; }
\^		{ return XOR; }
&=		{ return ANDEQUAL; }
&		{ return AND; }
\+=		{ return PLUSEQUAL; }
\+		{ printf("+ "); return PLUS; }
\-=		{ return MINUSEQUAL; }
-		{ return MINUS; }
\*\*=		{ return POWEREQUAL; }
\*\*		{ printf("** "); return POWER; }
\*		{ printf("* "); return MULT; }
\/\/=		{ return FLOORDIVEQUAL; }
\/\/		{ return FLOORDIVIDE; }
\/=		{ return DIVEQUAL; }
\/		{ return DIVIDE; }
~		{ return BINNOT; }
%=		{ return MODEQUAL; }
%		{ printf("%% "); return MODULO; }
\(		{ printf("LPAREN "); BEGIN INBRACK; brack_stack++; return LPAREN; }
\)		{ printf("RPAREN "); if(--brack_stack == 0) BEGIN 0; return RPAREN; }
\{		{ BEGIN INBRACK; brack_stack++; return LCURLY; }
\}		{ if(--brack_stack == 0) BEGIN 0; return RCURLY; }
\[		{ printf("LBRACK "); BEGIN INBRACK; brack_stack++; return LBRACK; }
\]		{ printf("RBRACK "); if(--brack_stack == 0) BEGIN 0; return RBRACK; }
\`		{ return SYSQUOTE; }
and		{ return ANDWORD; }	
or		{ return ORWORD; }
del		{ printf("DEL "); return DEL; }
pass		{ return PASS; }
yield		{ return YIELD; }
from		{ return FROM; }
import		{ return IMPORT; }
as		{ return AS; }
exec		{ return EXEC; }
global		{ return GLOBAL; }
assert		{ return ASSERT; }
raise		{ return RAISE; }
with		{ return WITH; }
def		{ printf("DEF "); return DEF; }
class		{ return CLASS; }
except		{ return EXCEPT; }
finally		{ return FINALLY; }
return		{ printf("RETURN"); return RETURN; }	
continue	{ return CONTINUE; }
break		{ return BREAK; }
print		{ printf("PRINT "); return PRINT; }
lambda		{ return LAMBDA; }
>=		{ return MORETHANOREQUAL; }
\<=		{ return LESSTHANOREQUAL; }
\<>		{ return EQUIV; }
\<\<=		{ return SHIFTLEFTEQUAL; }
\<\<		{ return SHIFTLEFT; }
>>=		{ return SHIFTRIGHTEQUAL; }
>>		{ return SHIFTRIGHT; }
>		{ printf("MORETHAN "); return MORETHAN; }
\<		{ printf("LESSTHAN "); return LESSTHAN; }
==		{ printf("EQUALTEST "); return EQUALTEST; }
!=		{ return NOTEQUAL; }
!		{ return NOT; }
=		{ printf("EQUALS "); return EQUALS; }
in		{ return IN; }
not		{ return NOTWORD; }
is		{ return IS; }
[0-9]+(L|l)	{ printf("NUMBER "); yy_val = atoi(yytext); return NUMBER; }
[0-9]+		{ printf("NUMBER "); yy_val = atoi(yytext); return NUMBER; }
[_a-zA-Z][_a-zA-Z0-9]*	{ 
			printf("NAME(%s) ", yytext); 
			yy_str = yytext; 
			yy_str_len = yyleng;
			return NAME;
		}
\'[^']*\'	{
			if(yytext[yyleng - 2] == '\\')
			{
				yyless(yyleng - 1);
				yymore();
			}
			else
			{
				printf("STRING(%s) ", yytext);
				yy_str = yytext;
				yy_str_len = yyleng;
				return STRING;
			}
		}
\"[^"^\n]*\"	{ 
			if(yytext[yyleng - 2] == '\\')
			{
				yyless(yyleng - 1);
				yymore();
			}
			else
			{
				printf("STRING(%s) ", yytext);
				yy_str = yytext;
				yy_str_len = yyleng;
				return STRING;
			}
		}
%%
int yywrap(void)
{ 
	if(longcomment)
		free(longcomment);
	return 1;
}

void push_indent(int s)
{
	printf("push_indent %d", s);
	if(++indent_stack_i < INDENT_STACKSIZE)
		indent_stack[indent_stack_i] = s;
	else
	{
		printf("indent stack overflow\n");
		exit(-1);
	}
}

/* If we're here, there's at least one dedent */
void pop_back_indent(int s)
{
	printf("pop_back_indent %d\n", s);
	dedents = 1;
	while(indent_stack_i-- > -1)
	{
		if(indent_stack[indent_stack_i] == s)
		{
			BEGIN DEDENTING;
			printf("%d dedents\n", dedents);
			return;
		}
		dedents++;
	}
	printf("bad indentation\n");
	exit(-1);
}


