%{
	/* Some of this is the grammer listed in python 
	 * distribution GRAMMAR file, adapted */
	#include <stdio.h>
	#include "rubygrammar.h"
	char * yy_str;
	int yy_str_len;
	int yy_val; 
	int yylex(void);
	void yyerror(char const *);

	#define RB(x)	ruby_grammar_preprocess(x)
%}

%token NUMBER 
%token NAME 
%token COMMENT 
%token IF
%token ELSEIF
%token ELSE
%token ANDWORD
%token ORWORD
%token OR
%token OREQUAL
%token XOR
%token XOREQUAL
%token AND
%token ANDEQUAL
%token SHIFTLEFT
%token SHIFTLEFTEQUAL
%token SHIFTRIGHT
%token SHIFTRIGHTEQUAL
%token PLUS
%token PLUSEQUAL
%token MINUS
%token MINUSEQUAL
%token MULT
%token MULTEQUAL
%token DIVIDE
%token DIVEQUAL
%token FLOORDIVIDE
%token FLOORDIVEQUAL
%token BINNOT
%token POWER
%token POWEREQUAL
%token MODULO
%token MODEQUAL
%token LPAREN
%token RPAREN
%token LCURLY
%token RCURLY
%token LBRACK
%token RBRACK
%token SYSQUOTE
%token NOT
%token NOTWORD
%token MORETHAN
%token LESSTHAN
%token EQUALTEST
%token EQUALS
%token MORETHANOREQUAL
%token LESSTHANOREQUAL
%token EQUIV
%token NOTEQUAL
%token IN
%token IS
%token RETURN
%token CONTINUE
%token BREAK
%token PASS
%token FROM
%token IMPORT
%token YIELD
%token WHILE
%token FOR
%token WITH
%token RAISE
%token GLOBAL
%token EXEC
%token EXCEPT
%token FINALLY
%token ASSERT
%token TRY
%token AS
%token DEL
%token PRINT
%token LAMBDA
%token CLASS
%token DEF
%token ATSYM
%token COLON
%token SEMICOLON
%token COMMA
%token PERIOD
%token INDENT
%token DEDENT
%token STRING
%token NEWLINE

%%


file_input: mult_stmt2; 
mult_stmt2: mult_stmt2 stmt
	| mult_stmt2 newline_comment 
	|
;
newline_comment: newline_comment NEWLINE COMMENT
	{
		RB(RB_NEWLINE);
		rb_str = yy_str;
		rb_str_len = yy_str_len;
		RB(RB_COMMENT);
	}
	| newline_comment NEWLINE { RB(RB_NEWLINE); }
	| COMMENT
	{
		rb_str = yy_str;
		rb_str_len = yy_str_len;
		RB(RB_COMMENT);
	}
	| NEWLINE { RB(RB_NEWLINE); }
;
stmt: simple_stmt | compound_stmt; 
mult_stmt: stmt
	| mult_stmt stmt
;
compound_stmt: if_stmt | while_stmt | for_stmt | try_stmt | with_stmt
	| funcdef | classdef | decorated;
if_stmt: IF { RB(RB_IF); } test COLON suite elseif_clause else_clause { RB(RB_END); RB(RB_NEWLINE);  }
while_stmt: WHILE { RB(RB_WHILE); }
	test COLON suite else_clause
	{
		RB(RB_END);
		RB(RB_NEWLINE);
	}
;
for_stmt: FOR exprlist IN testlist COLON suite else_clause;
try_stmt: TRY COLON suite mult_except_clause else_clause finally_clause
	| TRY COLON suite FINALLY COLON suite;
mult_except_clause: except_clause COLON suite
	| mult_except_clause except_clause COLON suite;
finally_clause:
	| FINALLY COLON suite;
except_clause: EXCEPT opt_test_ex;
opt_test_ex: test AS test | test COMMA test
	| test
	|
;
with_stmt: WITH test opt_with_var COLON suite;
opt_with_var: with_var
	|
;
with_var: AS expr;

elseif_clause: elseif_clause ELSEIF { RB(RB_ELSEIF); }
	test COLON suite
	|
;
else_clause: ELSE { RB(RB_ELSE); }
	COLON suite
	|
;
test: or_test ifelse
	| or_test
	| lambdef 
;
ifelse: IF or_test ELSE { RB(RB_ELSE); } test { RB(RB_END); RB(RB_NEWLINE); };
or_test: and_test mult_or;
mult_or: ORWORD and_test mult_or
	|
;
and_test: not_test mult_and; 
mult_and: ANDWORD not_test mult_and
	|
;
not_test: NOTWORD not_test | comparison;
comparison: expr mult_comp_op;
mult_comp_op: mult_comp_op comp_op expr
	|
;
comp_op: LESSTHAN { RB(RB_LESSTHAN); }
	| MORETHAN  { RB(RB_MORETHAN); }
	| EQUALTEST { RB(RB_EQUALTEST); }
	| MORETHANOREQUAL { RB(RB_MORETHANOREQUAL); }
	| LESSTHANOREQUAL { RB(RB_LESSTHANOREQUAL); }
	| EQUIV 
	| NOTEQUAL 	  { RB(RB_NOTEQUAL); }
	| IN | NOTWORD IN
	| IS | IS NOTWORD;
expr: xor_expr mult_xor_expr;
mult_xor_expr: mult_xor_expr OR xor_expr
	|
;
xor_expr: and_expr mult_and_expr;
mult_and_expr: mult_and_expr XOR and_expr
	|
;
and_expr: shift_expr mult_shift_expr;
mult_shift_expr: mult_shift_expr AND shift_expr
	|
;
shift_expr: arith_expr mult_arith_expr;
mult_arith_expr: mult_arith_expr SHIFTLEFT arith_expr
	| mult_arith_expr SHIFTRIGHT arith_expr
	|
;
arith_expr: term mult_term;
mult_term: plus_minus_term mult_term
	|
;
plus_minus_term: PLUS { RB(RB_PLUS); } term
	| MINUS { RB(RB_MINUS); } term
;
term: factor mult_factor;
mult_factor: mult_factor MULT { RB(RB_MULT); } factor
	| mult_factor DIVIDE { RB(RB_DIVIDE); } factor
	| mult_factor MODULO { RB(RB_MODULO); } factor
	| mult_factor FLOORDIVIDE factor
	|
;
factor: PLUS { RB(RB_PLUS); } factor
	| MINUS { RB(RB_MINUS); } factor
	| BINNOT factor
	| power;
power: atom mult_trailer opt_factor;
mult_trailer: mult_trailer trailer
	|
;
opt_factor: POWER factor
	|
;
atom: LPAREN { RB(RB_LPAREN); } opt_yield_test RPAREN { RB(RB_RPAREN); } 
	| LBRACK { RB(RB_LBRACK); } opt_listmaker RBRACK { RB(RB_RBRACK); } 
	| LCURLY { RB(RB_LCURLY); } opt_dictmaker RCURLY { RB(RB_RCURLY); } 
	| SYSQUOTE testlist1 SYSQUOTE 
	| NAME { rb_str = yy_str; rb_str_len = yy_str_len; RB(RB_NAME); } 
	| NUMBER { rb_val = yy_val; RB(RB_NUMBER); }
	| STRING mult_string { rb_str = yy_str; rb_str_len = yy_str_len; RB(RB_STRING); }
;
opt_yield_test: yield_expr | testlist_gexp
	|
;
opt_listmaker: listmaker 
	|
;
opt_dictmaker: dictmaker
	|
;
mult_string: mult_string STRING { }
	|
;
listmaker: test list_for | test mult_test opt_comma ;
mult_test: test COMMA { RB(RB_COMMA); } mult_test 
	|
;
opt_comma: COMMA { RB(RB_COMMA); }
	|
;
testlist_gexp: test gen_for | test mult_test opt_comma;
lambdef: LAMBDA varargslist COLON test;
trailer: LPAREN 
	{ RB(RB_LPAREN); }
	opt_arglist 
	{ RB(RB_RPAREN); }
	RPAREN
	| LBRACK { RB(RB_LBRACK); } subscriptlist RBRACK { RB(RB_RBRACK); }
	| PERIOD NAME
	{
		RB(RB_PERIOD);
		rb_str = yy_str;
		rb_str_len = yy_str_len;
		RB(RB_NAME);
	}
;
opt_arglist: arglist
	|
;
subscriptlist: subscript mult_subscript opt_comma;
mult_subscript: mult_subscript COMMA subscript
	|
;
subscript: PERIOD PERIOD PERIOD | test | opt_test COLON opt_test opt_sliceop;
opt_test: test
	|
;
opt_sliceop: sliceop
	|
;
sliceop: COLON opt_test;
dictmaker: test COLON test mult_dictmaker opt_comma;
mult_dictmaker: mult_dictmaker COMMA test COLON test
	|
;
arglist: mult_arg argument opt_comma
	| MULT test mult_arg opt_power_test
	| POWER test;
mult_arg: mult_arg argument COMMA
	|
;
opt_power_test: COMMA POWER test
	|
;
argument: test opt_gen_for
	| test EQUALS { /*RB(RB_EQUALS);*/ } test
exprlist: expr mult_expr opt_comma;
mult_expr: mult_expr COMMA expr
	|
;

list_for: FOR exprlist IN testlist_safe opt_list_iter;
opt_list_iter:
	| list_iter;
list_iter: list_for | list_if;
list_if: IF old_test opt_list_iter;
opt_gen_for:
	| gen_for;
gen_for: FOR exprlist IN or_test opt_gen_iter;
opt_gen_iter:
	| gen_iter;
gen_iter: gen_for | gen_if;
gen_if: IF old_test opt_gen_iter;
testlist_safe: old_test opt_mult_old_test;
opt_mult_old_test: mult_old_test opt_comma
	|
;
mult_old_test: mult_old_test COMMA old_test;
	| COMMA old_test
;
old_test: or_test | old_lambdef;
old_lambdef: LAMBDA opt_lvarargslist;
opt_lvarargslist: COLON old_test
	| varargslist COLON old_test
;
testlist1: test mult_test;
yield_expr: YIELD { RB(PY_YIELD); } opt_testlist;
opt_testlist: testlist
	|
;
testlist: test mult_test opt_comma;
suite: simple_stmt 
	| newline_comment INDENT 
	mult_stmt 
	DEDENT
	{ 
	}
;
simple_stmt: small_stmt another_small_stmt 
	SEMICOLON { RB(RB_SEMICOLON); } newline_comment 
	| small_stmt another_small_stmt
	newline_comment 
;
another_small_stmt: another_small_stmt SEMICOLON 
	{ RB(RB_SEMICOLON); }
	small_stmt
	|
;
small_stmt: expr_stmt | print_stmt | del_stmt | pass_stmt | flow_stmt
	| import_stmt | global_stmt | exec_stmt | assert_stmt;
expr_stmt: testlist augassign yield_or_test 
	| testlist mult_yet;
yield_or_test: yield_expr | testlist;
mult_yet: mult_yet EQUALS { RB(RB_EQUALS); } yield_or_test
	|
;
augassign: PLUSEQUAL | MINUSEQUAL | MULTEQUAL | DIVEQUAL | MODEQUAL
	| ANDEQUAL | OREQUAL | XOREQUAL | SHIFTLEFTEQUAL
	| SHIFTRIGHTEQUAL | POWEREQUAL | FLOORDIVEQUAL;
print_stmt: PRINT { RB(PY_PRINT); } opt_mult_test2
	| PRINT SHIFTRIGHT { }
	test opt_mult_test
;
opt_mult_test2: test mult_test opt_comma
	|
;
opt_mult_test: comma_test opt_comma
	|
;
comma_test: comma_test COMMA { RB(RB_COMMA); } test
	| COMMA { RB(RB_COMMA); } test;
del_stmt: DEL exprlist;
pass_stmt: PASS;
flow_stmt: BREAK  { RB(RB_BREAK); } 
	| CONTINUE { RB(RB_CONTINUE); }
	| RETURN { RB(RB_RETURN); }
	opt_testlist
	| raise_stmt
	| yield_expr;
raise_stmt: RAISE test COMMA test COMMA test
	| RAISE test COMMA test
	| RAISE test
	| RAISE
;
import_stmt: import_name | import_from;
import_name: IMPORT dotted_as_names
import_from: FROM mult_period dotted_name import_from_list 
	| FROM PERIOD mult_period import_from_list;
import_from_list: IMPORT import_as_names | IMPORT MULT
	| IMPORT LPAREN import_as_names RPAREN;
mult_period:
	| mult_period PERIOD;
import_as_name: NAME | NAME AS NAME;
dotted_as_name: dotted_name | dotted_name AS NAME;
import_as_names: import_as_name mult_import_as_name opt_comma;
mult_import_as_name:
	| mult_import_as_name COMMA import_as_name;
dotted_as_names: dotted_as_name mult_dotted_as_name;
mult_dotted_as_name:
	| mult_dotted_as_name COMMA dotted_as_name;
dotted_name: NAME mult_dot_name;
mult_dot_name:
	| mult_dot_name PERIOD NAME;
global_stmt: GLOBAL { RB(PY_GLOBAL); } NAME mult_comma_name;
mult_comma_name:
	| mult_comma_name COMMA { RB(RB_COMMA); } NAME;
exec_stmt: EXEC expr | EXEC expr IN test | EXEC expr IN test COMMA test;
assert_stmt: ASSERT test | ASSERT test COMMA test;

funcdef: DEF NAME 
	{ 
		RB(RB_DEF);
		rb_str = yy_str; rb_str_len = yy_str_len;
		RB(RB_NAME);
	}
	parameters COLON
	suite
	{
		printf("Function end here!\n");
		RB(RB_END);
		RB(RB_NEWLINE);
	};
parameters: LPAREN 
	{ RB(RB_LPAREN); }
	opt_varargslist 
;
opt_varargslist: RPAREN { RB(RB_RPAREN); }
	| varargslist RPAREN { RB(RB_RPAREN); }
varargslist: mult_fpdef MULT NAME
	{
		RB(RB_DEREF);
		rb_str = yy_str; rb_str_len = yy_str_len;
		RB(RB_NAME);

	}
	| mult_fpdef POWER NAME
	{
		RB(RB_DEREF2);
		rb_str = yy_str; rb_str_len = yy_str_len;
		RB(RB_NAME);
	}
	| mult_fpdef opt_fpdef_equal
;
mult_fpdef: mult_fpdef fpdef opt_equal_test COMMA
	{ RB(RB_COMMA); }
	|
;

opt_fpdef_equal: fpdef opt_equal_test
	|
;
opt_equal_test: EQUALS { /* func equals */ RB(RB_EQUALS); } test
	|
;
fpdef: NAME  
	{ 
		rb_str = yy_str;
		rb_str_len = yy_str_len;
		RB(RB_NAME);
	}
	| LPAREN fplist RPAREN;
fplist: fpdef mult_fpdef3 opt_comma;
mult_fpdef3: mult_fpdef3 COMMA fpdef
	|
;

classdef: CLASS NAME 
	{
		RB(RB_CLASS);
		rb_str = yy_str;
		rb_str_len = yy_str_len;
		RB(RB_NAME);
	}
	opt_opt_testlist COLON suite;
opt_opt_testlist: LPAREN opt_testlist RPAREN
	|
;

decorator: ATSYM dotted_name opt_opt_arglist newline_comment; 
opt_opt_arglist: LPAREN opt_arglist RPAREN
	|
;
decorators: decorators decorator
	| decorator;
decorated: decorators classdef | decorators funcdef;


%%

void yyerror(char const * s)
{
	fprintf(stderr, "%s\n", s);
}
