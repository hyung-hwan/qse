{ 
	line[NR] = $0;
}

END { 
	i = NR;
	for (i = NR; i > 0; i = i - 1) print line[i];
}
