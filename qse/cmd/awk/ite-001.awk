{
	i = 1;
	while (i <= $3) {
		printf ("\t%.2f\n", $1*(1+$2)**i);
		i = i + 1;
	}
}
