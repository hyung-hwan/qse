BEGIN {
	for (i = -10.0; i < 10.0; i++)
	{
		print sin(i);
		print cos(i);
		print tan(i);
		print atan(i);
		print atan2(i, 1);
		print log(i);
		print exp(i);
		print sqrt(i);
	}
}
