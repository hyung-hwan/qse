#
# depending on where a variable name appeared, a variable name can be
# a global variable or a named variable.
#

function fn () 
{ 
	a = 20; 
	return a;
}

@global a;

BEGIN { 
	a = 30
	print fn ()
	print a 
}

