
function abc (x)
{
	local x;

	x = 20;
	print x;
	abc ();
}

BEGIN {
	abc ();
}
