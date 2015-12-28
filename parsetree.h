#include "objects.h"

struct parse_tree
{
	int type;
	int val;
	char * name;
	object * object;
	int branches;
	int parent_index;
	struct parse_tree ** branch;
	struct parse_tree * parent;
};

struct parse_tree prog_pt;

