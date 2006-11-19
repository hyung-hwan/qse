function main ()
{
	local i, k, c;

	for (i = 0; i < 10; i++)
	{
		abc[i*2] = i;
	}

	k = 20;

	for (i = 0; i < 100; i++)
	{
		if (i in abc) j[i] = i;
	}

	print "end of program";
}
