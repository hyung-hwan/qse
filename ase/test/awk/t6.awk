BEGIN {
	j = -20;

	for (i = -10; i < 10; i++) 
	{
		if (i == 5) exit; 
		//if (i == 5) break;
	}


       	while (j < 10) 
	{
		if (j == 5) break;
		j++;
	}
}

END {
	print "i = ", i;
	print "j = ", j;
}
