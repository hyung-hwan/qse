BEGIN {
	a[1,2,3] = 20;
	a[4,5,6] = 30;

	for (i in a)
	{
		n = split (i, k, SUBSEP);
		for (j = 1; j < n; j++)
		{
			print k[j]
		}
	}

	if ((1,2,3) in a)
	{
		print a[1,2,3];
	}	
}
