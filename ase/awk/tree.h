
enum TokenType
{
	IF, END, ID, NUM, READ, WrITE, UNTIL, ....
}


enum NodeKind { statement, expression };
enum StatKind { if, repeat, assign, read, write };
enum ExpKind  { op, const, id };
enum ExpType  {  void, integer, boolean };

struct treenode
{
	treenode* child[3]; // 
	treenode* sibling; // <---- next statement...

	int lineno;
	NodeKind node_kind;	
	union {
		statkind s;
		expkind e;
	} kind;
	union {
		TokenType Op;
		int val;
		char* name;
	} attr;


	exptype type; <- for type checking...
};
