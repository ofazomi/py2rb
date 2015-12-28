#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rubygrammar.h"
#include "parsetree.h"

#define TOKENSTACK_SIZE		100

int tokenstack[TOKENSTACK_SIZE];

int tokenstack_b = 0;
int tokenstack_e = 0;

int state = 0;

extern FILE * outscript;

struct parse_tree * add_branch(struct parse_tree * t);
void remove_last_branch(struct parse_tree * t);
void remove_incl_branches(struct parse_tree * t, int b, int e);
void insert_branches(struct parse_tree * pt, int i, int n);
struct parse_tree * get_prev_branch(struct parse_tree * t);
void take_off_block(struct parse_tree * t);
void reorder_extra_newlines(struct parse_tree * t);
void process_grammar(struct parse_tree * pt);
void assign_name(struct parse_tree * pt, const char * s);
void process_tokens(void);
void get_variable_scopes(struct parse_tree * pt);
object * getVariable(struct parse_tree * pt);
object * findObject(char * name);
char * getClassName(char * name, int len);
char * lookup_name(char * name);
void add_name(char * old_name, char * new_name);
void freeNameTables(void);

char ** nameTableFrom = NULL;
char ** nameTableTo = NULL;
int nameTableSize = 0;

int current_branch_index;

struct parse_tree * pt;

void init_ruby_grammar(void)
{
	prog_pt.type = RB_PROG;
	prog_pt.parent = NULL;
	prog_pt.branches = 0;
	prog_pt.branch = NULL;
	prog_pt.name = NULL;
	prog_pt.val = 0;
	pt = &prog_pt;
	current_branch_index = -1;
	objects = 0;
	objectList = NULL;
}

/* Called by python.l generated code... in case you don't work on this for three years
 * and forget again: RB(x) 
 * This does all the stuff we can do one token at a time.*/ 
/* Actually it also creates the parse tree and I'm not sure what else
 * some newline fixing but besides that... */
void ruby_grammar_preprocess(int token)
{
	struct parse_tree * t;
	pt = add_branch(pt);
	current_branch_index++;
	pt->type = token;
	switch(token)
	{
		case RB_DEF:
			current_branch_index = -1;
			break;
		case RB_CLASS:
			current_branch_index = -1;
			break;
		case RB_IF:
			current_branch_index = -1;
			break;
		case RB_ELSEIF:
			//t = pt->parent;
			remove_last_branch(pt->parent);
			//pt = t;
			get_prev_branch(pt)->val++;
			pt = pt->parent->parent;
			current_branch_index = -1;
			break;
		case RB_ELSE:
			//t = pt->parent;
			//remove_last_branch(pt->parent);
			//pt = t;
			get_prev_branch(pt)->val++;
			//printf("Previous newline value else: %d\n", get_prev_branch(pt)->val);
			pt = pt->parent->parent;
			current_branch_index = -1;
			break;
		case RB_WHILE:
			current_branch_index = -1;

			break;
		case RB_END:
			//reorder_extra_newlines(pt);
			get_prev_branch(pt)->val++;	//dedent
			//printf("Previous newline value end: %d\n", get_prev_branch(pt)->val);
			//take_off_block(pt);
			pt = pt->parent;
			break;
		case RB_STRING:
			pt->name = calloc(rb_str_len + 1, sizeof(char));
			strncpy(pt->name, rb_str, rb_str_len);
			pt->name[rb_str_len] = '\0';
			pt = pt->parent;	
			break;
		case RB_NAME:
			if(current_branch_index == 0 && pt->parent->type == RB_CLASS)
				pt->name = getClassName(rb_str, rb_str_len);
			else if(current_branch_index == 0 && pt->parent->type == RB_DEF)
				pt->name = getClassName(rb_str, rb_str_len);
			else
			{
				pt->name = calloc(rb_str_len + 1, sizeof(char));
				strncpy(pt->name, rb_str, rb_str_len);
				pt->name[rb_str_len] = '\0';
			}
			pt = pt->parent;	
			break;
		case RB_COMMENT:
			pt->name = calloc(rb_str_len + 1, sizeof(char));
			strncpy(pt->name, rb_str, rb_str_len);
			pt->name[rb_str_len] = '\0';
			printf("Comment! %s\n", pt->name);
			pt = pt->parent;	
			break;
		case RB_NUMBER:
			pt->val = rb_val;
			pt = pt->parent;	
			break;
		default:
			pt = pt->parent;	
			break;
	}
}

struct parse_tree * add_branch(struct parse_tree * t)
{
	struct parse_tree * n;
	t->branches++;
	t->branch = realloc(t->branch, t->branches * sizeof(struct parse_tree *));
	if(!t->branch)
		{ fprintf(stderr, "Ruby grammar can't reallocate parse tree\n"); exit(-1); }
	n = calloc(1, sizeof(struct parse_tree));
	if(!n)
		{ fprintf(stderr, "Ruby grammar can't allocate parse tree\n"); exit(-1); }
	t->branch[t->branches - 1] = n;
	n->parent = t;
	n->parent_index = t->branches - 1;
	n->branches = 0;
	n->branch = NULL;
	n->name = NULL;
	n->val = 0;
	//FIXME maybe an "unassigned type" init value?
	return n;
}

void remove_last_branch(struct parse_tree * t)
{
	if(!t->branches)
		{ fprintf(stderr, "Ruby grammar error: rlb\n"); exit(-1); }
	t->branches--;
	t->branch = realloc(t->branch, t->branches * sizeof(struct parse_tree *));
	if(!t->branch)
		{ fprintf(stderr, "Ruby grammar can't reallocate parse tree\n"); exit(-1); }
}

void remove_incl_branches(struct parse_tree * t, int b, int e)
{
	int i;
	if(b > e || e > t->branches)
		return;
	if(e == t->branches)
	{
		t->branches = b;
	}
	else
	{
		for(i = b; i < t->branches - (e - b) - 1; i++)
			t->branch[i] = t->branch[i + (e - b) + 1];
		t->branches -= (e - b) + 1;
	}
	t->branch = realloc(t->branch, t->branches * sizeof(struct parse_tree *));
	if(!t->branch)
		{ fprintf(stderr, "Ruby grammar can't reallocate parse tree\n"); exit(-1); }
}

void insert_branches(struct parse_tree * pt, int i, int n)
{
	int j;

	for(j = 0; j < n; j++)
		add_branch(pt);
	for(j = pt->branches - 1; j > i + n - 1; j--)
	{
		memcpy(pt->branch[j], pt->branch[j - n], sizeof(struct parse_tree));
		pt->branch[j]->parent_index = j;
	}
}

/* This is basically used for adding dedents to the previous newline */
struct parse_tree * get_prev_branch(struct parse_tree * t)
{
	if(t->parent_index == 0)
		{ fprintf(stderr, "Ruby grammar error: gpb\n"); exit(-1); }
	if(!t->parent)
		{ fprintf(stderr, "Ruby grammar error: no parent\n"); exit(-1); }
	if(current_branch_index >= t->parent->branches + 1)
		{ printf("god damn it\n"); exit(-1); }
	return t->parent->branch[t->parent_index - 1];
}

void take_off_block(struct parse_tree * t)
{
	current_branch_index = t->parent->parent->branches - 1;
	printf("Setting cbi to %d\n", t->parent->branches);
}

void reorder_extra_newlines(struct parse_tree * t)
{
	int i, n;
	struct parse_tree * new_b;
	n = -1;
	t = t->parent;
	i = t->branches - 2;	//assume branches - 1 is the "end"
	while(i > -1 && t->branch[i]->type == RB_NEWLINE)
	{
		n++;
		i--;
	}
	/* Apparently n = -1 above is not so clever, for instance
	 * if we have an end right after an if block with no newline inbetween */
	if(n == -1)
		return;
	if(n > 1)
		printf("Reordering %d newlines\n", n);
	remove_incl_branches(t, t->branches - 2 - n, t->branches - 3);
	t = t->parent;
	current_branch_index -= n;
	while(n--)
	{
		new_b = add_branch(t);
		new_b->type = RB_NEWLINE;
	}
}

/* First thing to do, find return statements that are not followed
 * by newlines and set their "val" to 1 */
void ruby_grammar_process(void)
{
	get_variable_scopes(&prog_pt);

	process_grammar(&prog_pt);
}

void process_grammar(struct parse_tree * pt)
{
	struct parse_tree * next_branch;
	int i, j;
	int paren_count, brack_count;

	for(i = 0; i < pt->branches; i++)
	{
		/* Actually I guess not, I do this in get_variable_scopes too.
		 * If I ever actually use the multiple branches for things like scopes
		 * or blocks or whatever else, which I might not, then its silly to do
		 * this and process the subbranches before the type of the branch itself.
		 * which highlights that I'll probably never use that and should remove it.
		 * I think I added it so that I could do some complex things like manipulating
		 * blocks of code and taking chunks out to replace them... maybe its necessary
		 * with yield, not sure, otherwise I should remove it.  FIXME */
		process_grammar(pt->branch[i]);	//i guess?
		if(i < pt->branches - 1)
			next_branch = pt->branch[i + 1];
		else
			next_branch = NULL;
		switch(pt->branch[i]->type)
		{
			case RB_RETURN:
				if(next_branch && next_branch->type != RB_NEWLINE)
					pt->branch[i]->val++;
				break;
			case PY_PRINT:
				j = i;
				paren_count = 0;
				brack_count = 0;
				while(j < pt->branches - 1)
				{
					j++;
					if(pt->branch[j]->type == RB_COMMA && paren_count == 0 && brack_count == 0)
					{
						pt->branch[i]->type = RB_NAME;
						/*if(pt->branch[i + 1]->type == RB_LBRACK || 
							(pt->branch[i + 1]->type == OBJECT && pt->branch[i + 1]->object->type == LIST))
						{
							insert_branches(pt, i + 1, 2);		
							assign_name(pt->branch[i], "Py2rbHelper");
							pt->branch[i + 1]->type = RB_PERIOD;
							pt->branch[i + 2]->type = RB_NAME;
							assign_name(pt->branch[i + 2], "print_pyformat_list");
							j += 2;
						}
						else*/
							assign_name(pt->branch[i], "print");
						pt->branch[j]->type = RB_SEMICOLON;	
						insert_branches(pt, j + 1, 2);
						pt->branch[j + 1]->type = RB_NAME;
						assign_name(pt->branch[j + 1], "print");
						pt->branch[j + 2]->type = RB_STRING;
						assign_name(pt->branch[j + 2], "\" \"");
						i = j + 3;
						break;
					}
					else if(pt->branch[j]->type == RB_LPAREN)
						paren_count++;
					else if(pt->branch[j]->type == RB_LBRACK)
						brack_count++;
					else if(pt->branch[j]->type == RB_RPAREN)
						paren_count--;
					else if(pt->branch[j]->type == RB_RBRACK)
						brack_count--;
					else if(pt->branch[j]->type == RB_NEWLINE ||
						pt->branch[j]->type == RB_SEMICOLON ||
						j == pt->branches - 1)
					{
						pt->branch[i]->type = RB_NAME;
						/*if(pt->branch[i + 1]->type == RB_LBRACK ||
							(pt->branch[i + 1]->type == OBJECT && pt->branch[i + 1]->object->type == LIST))
						{
							insert_branches(pt, i + 1, 2);		
							assign_name(pt->branch[i], "Py2rbHelper");
							pt->branch[i + 1]->type = RB_PERIOD;
							pt->branch[i + 2]->type = RB_NAME;
							assign_name(pt->branch[i + 2], "puts_pyformat_list");
						}
						else*/
							assign_name(pt->branch[i], "puts");
						break;
					}
				}
				break;
			case RB_EQUALS:
				if(next_branch && next_branch->type == RB_LPAREN)
				{
					if(i > 0 && pt->branch[i - 1]->type == OBJECT)
						pt->branch[i - 1]->object->type = LIST;
					//if(i > 0 && pt->branch[i - 1]->type == RB_NAME)
					//	pt->branch[i - 1]->ext_type = LIST;	//actually a python sequence or tuple but...
				}
				else if(next_branch && next_branch->type == RB_LBRACK)
				{
					if(i > 0 && pt->branch[i - 1]->type == OBJECT)
						pt->branch[i - 1]->object->type = LIST;
					//if(i > 0 && pt->branch[i - 1]->type == RB_NAME)
					//	pt->branch[i - 1]->ext_type = LIST;
				}
			case RB_PLUS:
			case RB_MINUS:
				if(next_branch && next_branch->type == RB_LPAREN)
				{
					paren_count = 0;
					brack_count = 0;
					j = i;
					while(j < pt->branches - 1)
					{
						j++;
						if(pt->branch[j]->type == RB_LPAREN)
							paren_count++;
						else if(pt->branch[j]->type == RB_LBRACK)
							brack_count++;
						else if(pt->branch[j]->type == RB_RPAREN)
							paren_count--;
						else if(pt->branch[j]->type == RB_RBRACK)
							brack_count--;

						if(paren_count == 0 && brack_count == 0)
						{
							pt->branch[i + 1]->type = RB_LBRACK;
							pt->branch[j]->type = RB_RBRACK;
							break;
						}
					}
				}
				break;
			case RB_COMMA:
				/* When we're sure that the commas aren't used for anything else,
				 * then we just find a comma, take the previous tokens off of the tree
				 * and put them on a stack, and so on, until we find an equals sign
				 * in which case, if there's a comma stack, we start putting the tokens
				 * back on, changing commas to semi colons.
				 * We need to, though, make the LPAREN its on block and similar things
				 * to ensure we don't break anything with this comma stack */
				/* So python does all kinds of stuff with commas:
					- a comma after a variable makes it into a set
					- a comma after a print statement bypasses the newline*/
				break;
		}
	}
}

/* FIXME use this in ruby_grammar_preprocess */
void assign_name(struct parse_tree * pt, const char * s)
{
	pt->name = calloc(strlen(s) + 1, sizeof(char));
	strncpy(pt->name, s, strlen(s));
	pt->name[strlen(s)] = '\0';
}

void process_tokens(void)
{
	static int reprocess_index = -1;
	static int indent_level = 0;

	while(tokenstack_b < tokenstack_e)
	{

		/* If its not a newline or an end
		 * we put the newlines back in */
		switch(tokenstack[tokenstack_b])
		{
			case RB_NEWLINE:
			case RB_END:
				break;
			case RB_ELSEIF:
			case RB_ELSE:
				if(state == RB_NEWLINE)
				{
					indent_level--;
					state = 0;
					newlines(tokenstack_b - reprocess_index);
					reprocess_index = -1;
					indents(indent_level);
				}
				break;
			default:
				if(state == RB_NEWLINE)
				{
					state = 0;
					newlines(tokenstack_b - reprocess_index);
					reprocess_index = -1;
					indents(indent_level);
				}
				break;
		}
		switch(tokenstack[tokenstack_b])
		{
			case RB_NEWLINE:
				if(reprocess_index == -1)
				{
					reprocess_index = tokenstack_b;
					state = RB_NEWLINE;
				}
				break;
			case RB_END:
				indent_level--;
				fputc('\n', outscript);
				indents(indent_level);
				fprintf(outscript, "end");
				break;
			case RB_DEF:
				fprintf(outscript, "def ");
				indent_level++;
				break;
			case RB_CLASS:
				fprintf(outscript, "class ");
				state = RB_CLASS;
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
				fprintf(outscript, "elsif ");
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
				break;
			
			/*case RB_PRINT:
				fprintf(outscript, "print ");
				break;*/

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
			case RB_NAME:
				if(state == RB_CLASS)
				{
					/* class names have to be capitalized without
					 * collisions.  Also, we may need to know
					 * when we're in a class for some reason */
					state = 0;
					fprintf(outscript, "%s", getClassName(rb_str, rb_str_len));
				}
				/* We also need special handling probably
				 * for function names and instance variables
				 * within a class.  The latter might be tricky.
				 * its even possible that we might have to store
				 * each whole class on the token stack before
				 * we can understand what names are
				 * local, instance, class, global, or constant.
				 * Or maybe its roughly the same and we can just
				 * throw it together.  I'd kind of like to do
				 * more interesting stuff and we could also just
				 * have an "abstract parse tree" for certain
				 * things, like classes, so we could add tokens
				 * to the tree here, and then when we hit the
				 * end, run another printer on the parse tree for
				 * the class.  Then this function would basically
				 * just be for taking tokens off the stack and
				 * doing indentation.  But then again, we're also
				 * doing spaces and things here, so that would
				 * have to be capable of being added to the
				 * parse tree and then I wonder if we wouldn't
				 * just want to make the whole program a parse
				 * tree with strings and print it out at the
				 * end.
				 * But again, all depends on how tricky
				 * this gets.  We still need variable mapping
				 * so we can start with that. */
			case RB_STRING:
				/* We might want to check here to see
				 * if this exact string has been used before
				 * and if its, say, more than 3 characters and
				 * if it is, replace it with a :symbol, assuming
				 * this is the proper use of symbols ... 
				 * But this would require editing the first
				 * and so it requires a parse tree.*/
				fwrite(rb_str, 1, rb_str_len, outscript);
				break;
			case RB_NUMBER:
				fprintf(outscript, "%d", rb_val);
				break;
			case RB_COMMENT:
				/* comments are handled by python lex
				 * right now */
				break;
			default:
				break;
		}
	}
	
}

void get_variable_scopes(struct parse_tree * pt)
{
	int i;
	for(i = 0; i < pt->branches; i++)
	{
		process_grammar(pt->branch[i]);	//i guess?
		switch(pt->branch[i]->type)
		{
			case RB_NAME:
				//what about classes
				getVariable(pt->branch[i]);
				break;
			case RB_EQUALS:
				//the second part can still do the LIST assign
				break;
		}
	}
}

/* I think a separate function can translate the name and fix any scoping issues 
 * I'm feeling a bit lazy right now, I'll probably fix this up later */
object * getVariable(struct parse_tree * pt)
{
	object * o;

	o = findObject(pt->name);
	if(!o)
	{
		o = calloc(1, sizeof(object));
		if(!o)
			{ fprintf(stderr, "py2rb can't allocate memory for object!\n"); exit(-1); } 
		o->name = pt->name;
		o->type = UNTYPED;
		objects++;
		objectList = realloc(objectList, objects * sizeof(object *));
		if(!objectList)
			{ fprintf(stderr, "py2rb can't allocate memory for object list!\n"); exit(-1); } 
		objectList[objects - 1] = o;
	}
	else
		free(pt->name);

	pt->type = OBJECT;
	pt->name = NULL;
	pt->object = o;
	return o;		//necessary ??
}

object * findObject(char * name)
{
	unsigned long i;
	int j;
	unsigned char c;

	for(i = 0; i < objects; i++)
	{
		j = 0;
		while(name[j])
		{
			if(!objectList[i]->name[j] || objectList[i]->name[j] != name[j])
				break; 
			j++;
		}
		if(!name[j])
			return objectList[i];
	}	
	return NULL;
}

/* We need to have two lists that we use to map
 * variables between, just for this, so that
 * every instance of a variable can be correctly
 * changed.
 * "name" is volatile here*/
char * getClassName(char * name, int len)
{
	int i;
	char * found_name;
	char * f;
	char first_char;
	char * new_name = malloc(len + 1);
	if(!new_name)
		{ fprintf(stderr, "py2rb can't allocate space for name!\n"); exit(-1); }
	strncpy(new_name, name, len); 
	new_name[len] = '\0';
	found_name = lookup_name(new_name);
	if(!found_name)
	{
		found_name = malloc(len + 1);
		strncpy(found_name, new_name, len + 1); 
		/* Convert to class name */
		first_char = found_name[0];
		if(first_char >= 'A' && first_char <= 'Z') {}
		else
		{
			if(first_char >= 'a' && first_char <= 'z')
			{
				found_name[0] -= 'a' - 'A';
			}
			else
			{
				found_name = realloc(found_name, (len + 2) * sizeof(char));
				for(i = len + 1; i > 0; i--)
					found_name[i] = found_name[i - 1];
				found_name[0] = 'C';
			}
			if(lookup_name(found_name) != 0)
			{
				found_name = realloc(found_name, (len + 3) * sizeof(char));
				found_name[len + 2] = '\0';
				found_name[len + 1] = '\0';
				found_name[len] = '1';
				while(lookup_name(found_name) != 0)
				{
					if(found_name[len + 1] == '\0')
					{
						found_name[len]++;	
						if(found_name[len] == '9' + 1)
						{
							found_name[len] = '1';
							found_name[len + 1] = '0';
						}
					}
					else
					{
						found_name[len + 1]++;	
						if(found_name[len + 1] == '9' + 1)
						{
							if(found_name[len + 1] == '9')
							{
								fprintf(stderr, "py2rb can't find acceptable name for '%s'\n", new_name);
								exit(-1);
							}
							found_name[len]++;
							found_name[len + 1] = '0';
						}
					}
				}
			}
		}
		add_name(new_name, found_name);
		return found_name;
	}
	else if(found_name == (char *)1)
		return new_name;
	else
		return found_name;
}

/* Returns 0 if not found.  1 also means unchanged*/
char * lookup_name(char * name)
{
	int i;
	for(i = 0; i < nameTableSize; i++)
	{
		if(strcmp(name, nameTableFrom[i]) == 0)
			return nameTableTo[i];
	}
	return NULL;
}

/* new_name can equal 1, meaning no change */
void add_name(char * old_name, char * new_name)
{
	nameTableSize++;
	nameTableFrom = realloc(nameTableFrom, nameTableSize * sizeof(char *));
	if(!nameTableFrom)
		{ fprintf(stderr, "Ruby printer can't realloc name table\n"); exit(-1); }
	nameTableTo = realloc(nameTableTo, nameTableSize * sizeof(char *));
	if(!nameTableTo)
		{ fprintf(stderr, "Ruby printer can't realloc name table\n"); exit(-1); }
	nameTableFrom[nameTableSize - 1] = old_name;
	nameTableTo[nameTableSize - 1] = new_name;
}

/* FIXME change name, this also frees object lists */
void freeNameTables(void)
{
	int i;
	unsigned long j;
	for(i = 0; i < nameTableSize; i++)
	{
		free(nameTableFrom[i]);
		if(nameTableTo[i] != (char *)1)
			free(nameTableTo[i]);
	}
	if(nameTableSize != 0)
	{
		free(nameTableTo);
		free(nameTableFrom);
	}
	for(j = 0; j < objects; j++)
	{
		if(objectList[j]->name)
			free(objectList[j]->name);
		free(objectList[j]);
	}
	free(objectList);
}
