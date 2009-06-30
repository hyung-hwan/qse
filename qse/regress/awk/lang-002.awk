#
# deep recursion must be detected if it goes too deep.
#

function f(x)
{
	print x;
	f("my hello");
}

BEGIN { f(10); }
