/* This might be the only "object" type, I'm just starting out here, maybe
 * this file should be called variables instead or maybe just objects */
typedef struct object {
	char * name;
	enum _type { UNTYPED, FIXNUM, LIST } type;
	//scope? start end token?

} object;

#define OBJECT			5000
object ** objectList;
unsigned long objects;
