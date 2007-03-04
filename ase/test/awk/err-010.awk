
global abc;

func abc ()
{
	local abc;

	abc = 20;
	print abc;
	abc ();
}

BEGIN {
	abc ();
}
