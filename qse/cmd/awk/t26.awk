function call_next ()
{
	next;
}

BEGIN {
	#call_next ();
}

END {
	call_next ();
}

