
function abc ()
{
	local j ;

	j = 20;
	print j;
	abc ();
}

global abc; 
BEGIN {
	abc ();
}
