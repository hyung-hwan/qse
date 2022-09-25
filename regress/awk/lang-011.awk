BEGIN {
	a[1,2,3] = 20;
	a[4,5,6] = 30;

	for (i in a)
	{
		n = split (i, k, SUBSEP);
		for (j = 1; j <= n; j++)
		{
			print k[j]
		}
		print "-------------------"
	}

	if ((1,2,3) in a)
	{
		print "(1,2,3) in a ==> " a[1,2,3];
	}	
	else
	{
		print "(1,2,3) not in a"
	}

	if ((4,5) in a)
	{
		print "(4,5) in a ==> " a[4,5]
	}
	else
	{
		print "(4,5) not in a"
	}
}
