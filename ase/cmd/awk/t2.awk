global z, x;

function sum (i)
{
	local k, y;

	y = 0;
	for (k = i; k; k = k - 1)
	{
		y = y + k;
	}

	return y;
	y = 10;
	return y;
}

END {
	/*x = sum (10000000);
	*/
	x = sum (100);
	s = x;
	ss = z;
}
