#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rubyprinter.h"

#ifdef DEBUG
#define inline	
#endif //DEBUG
int pstate = 0;

extern FILE * outscript;

void process_tokens(void);
inline void indents(int n);
inline void newlines(int n);

void ruby_print(struct parse_tree * pt)
{
	static int reprocess_index = -1;
	static int indent_level = 0;
	int i;
	if(!pt)
		pt = &prog_pt;

	switch(pt->type)
	{
		case RB_NEWLINE:
		case RB_END:
			break;
		case RB_ELSEIF:
		case RB_ELSE:
			if(pstate == RB_NEWLINE)
			{
				//indent_level--;
				//pstate = 0;
				//reprocess_index = -1;
				//indents(indent_level);
			}
			break;
		default:
			if(pstate == RB_NEWLINE)
			{
				pstate = 0;
				reprocess_index = -1;
				//indents(indent_level);
			}
			break;
	}
	switch(pt->type)
	{
		case RB_NEWLINE:
			if(reprocess_index == -1)
			{
				pstate = RB_NEWLINE;
			}
			fputc('\n', outscript);
			indent_level -= pt->val;
			indents(indent_level);
			break;
		case RB_COMMENT:
			fprintf(outscript, "%s", pt->name);
			break;
		case RB_END:
			fprintf(outscript, "end");
			break;
		case RB_DEF:
			fprintf(outscript, "def ");
			indent_level++;
			break;
		case RB_CLASS:
			fprintf(outscript, "class ");
			pstate = RB_CLASS;
			indent_level++;
			break;
		case RB_IF:
			fprintf(outscript, "if ");
			indent_level++;
			break;
		case RB_ELSEIF:
			fprintf(outscript, "elsif ");
			indent_level++;
			break;
		case RB_ELSE:
			fprintf(outscript, "else ");
			indent_level++;
			break;
		case RB_WHILE:
			fprintf(outscript, "while ");
			indent_level++;
			break;
		case RB_BREAK:
			fprintf(outscript, "break");
			break;
		case RB_CONTINUE:
			fprintf(outscript, "continue");
			break;
		case RB_RETURN:
			fprintf(outscript, "return");
			if(pt->val)
				fputc(' ', outscript);
			break;
		case RB_LESSTHAN:
			fprintf(outscript, " < ");
			break;
		case RB_MORETHAN:
			fprintf(outscript, " > ");
			break;
		case RB_EQUALTEST:
			fprintf(outscript, " == ");
			break;
		case RB_MORETHANOREQUAL:
			fprintf(outscript, " >= ");
			break;
		case RB_LESSTHANOREQUAL:
			fprintf(outscript, " <= ");
			break;
		case RB_NOTEQUAL:
			fprintf(outscript, " != ");
			break;
		case RB_EQUALS:
			fprintf(outscript, " = ");
			break;
		case RB_LPAREN:
			fprintf(outscript, "(");
			break;
		case RB_RPAREN:
			fprintf(outscript, ")");
			break;
		case RB_LBRACK:
			fprintf(outscript, "[");
			break;
		case RB_RBRACK:
			fprintf(outscript, "]");
			break;
		case RB_LCURLY:
			fprintf(outscript, "{");
			break;
		case RB_RCURLY:
			fprintf(outscript, "}");
			break;
		case RB_PERIOD:
			fprintf(outscript, ".");
			break;
		case RB_PLUS:
			fprintf(outscript, " + ");
			break;
		case RB_MINUS:
			fprintf(outscript, " - ");
			break;
		case RB_MULT:
			fprintf(outscript, " * ");
			break;
		case RB_DIVIDE:
			fprintf(outscript, " / ");
			break;
		case RB_MODULO:
			fprintf(outscript, " % ");
			break;
		case RB_DEREF:
			fprintf(outscript, "*");
			break;
		case RB_DEREF2:		//fix this its python syntax, same for above FIXME
			fprintf(outscript, "**");
			break;
		case RB_SEMICOLON:
			fprintf(outscript, ";");
			if(pt->parent_index < pt->parent->branches - 1 && pt->parent->branch[pt->parent_index + 1]->type != RB_NEWLINE)
				fprintf(outscript, " ");
			break;
		case RB_COMMA:
			fprintf(outscript, ", ");
			break;
		case RB_NAME:
			fprintf(outscript, "%s", pt->name);
			if(pt->parent_index < pt->parent->branches - 1)
			{
				switch(pt->parent->branch[pt->parent_index + 1]->type)
				{
					case OBJECT:
					case RB_NAME:
					case RB_NUMBER:
						fprintf(outscript, " ");
						break;
					case RB_LBRACK:
						//if(pt->parent_index > 0 && pt->parent->branch[pt->parent_index - 1]->type == RB_PERIOD)
							fprintf(outscript, " ");
						break;
					default:
						break;
				}
			}
			break;
		case OBJECT:
			fprintf(outscript, "%s", pt->object->name);
			break;	
		case RB_STRING:
			/* We might want to check here to see
			 * if this exact string has been used before
			 * and if its, say, more than 3 characters and
			 * if it is, replace it with a :symbol, assuming
			 * this is the proper use of symbols ... 
			 * But this would require editing the first
			 * and so it requires a parse tree.*/
			fprintf(outscript, "%s", pt->name);
			break;
		case RB_NUMBER:
			fprintf(outscript, "%d", pt->val);
			break;
		default:
			break;
	}

	for(i = 0; i < pt->branches; i++)
	{
		ruby_print(pt->branch[i]);
	}
}

inline void indents(int n)
{
	int i;
	for(i = 0; i < n; i++)
		fputc('\t', outscript);
}

inline void newlines(int n)
{
	int i;
	for(i = 0; i < n; i++)
		fputc('\n', outscript);
}
