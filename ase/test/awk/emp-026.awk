{ 
	line[NR] = $0;
}

END { 
	i = NR;
	while (i > 0)
	{
		print line[i];
		i = i - 1;
	}
}
