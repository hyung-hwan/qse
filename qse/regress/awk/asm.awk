#
# $Id: asm.awk,v 1.4 2007/09/27 11:33:45 bacon Exp $
#
# Taken from the book "The AWK Programming Language"
#

BEGIN {
	srcfile = ARGV[1];
	ARGV[1] = "";
	tempfile = "asm.temp";
	n = split("const get put ld st add sub jpos jz j halt", x);
	for (i = 1; i <= n; i++) op[x[i]] = i - 1;

# PASS 1
	FS = "[ \t]+";
	while (getline <srcfile > 0) {
		sub (/#.*/, "");
		symtab[$1] = nextmem;

		if ($2 != "") {
			print $2 "\t" $3 >tempfile;
			nextmem++;
		}	
	}
	close (tempfile);

# PASS 2
	nextmem = 0;
	while (getline <tempfile > 0) {
		if ($2 !~ /^[0-9]*$/) $2 = symtab[$2];
		mem[nextmem++] = 1000 * op[$1] + $2;
	}

# INTERPRETER
	for (pc = 0; pc >= 0; ) {
		addr = mem[pc] % 1000;
		code = int(mem[pc++] / 1000);
		if      (code == op["get"])  { if (getline acc <= 0) acc = 0; }
		else if (code == op["put"])  { print acc; }
		else if (code == op["st"])   { mem[addr] = acc; }
		else if (code == op["ld"])   { acc = mem[addr]; }
		else if (code == op["add"])  { acc += mem[addr]; }
		else if (code == op["sub"])  { acc -= mem[addr]; }
		else if (code == op["jpos"]) { if (acc > 0) pc = addr; }
		else if (code == op["jz"])   { if (acc == 0) pc = addr; }
		else if (code == op["j"])    { pc = addr; }
		else if (code == op["halt"]) { pc = -1; }
		else                         { pc = -1; }
	}
}

