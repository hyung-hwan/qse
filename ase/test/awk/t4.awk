function main ()
{
	local i;

	for (i = 0; i < 10; i++)
	{
		abc[i*2] = i;
	}

	for (i = 0; i < 100; i++)
	{
		if (i in abc) j[i] = i;
	}
}
