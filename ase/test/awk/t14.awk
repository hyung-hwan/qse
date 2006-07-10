/*
{
	print "1111111111111";
}

{
	print "22222222222222";
	next;
}

{
	print "33333333333333333";
}

END
{
	print "\a";
}
*/

global x, y;
{
	for (i = 0; i < 10; i++)
	{
		print $i;
		if (i == 3) a = $i;
	}

	/*
	$1 = $2;
	$3 = $2;
	$2 = $2;
	print $0;
	*/
	print "--------------------";
	print NF;
	print "====================";
}
