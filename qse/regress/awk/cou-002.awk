BEGIN { 
	FS = "\t";
	printf  ("%10s %6s %5s    %s\n\n", 
		"COUNTRY", "AREA", "POP", "CONTINENT");
}

{
	printf ("%10s %6d %5d    %s\n", $1, $2, $3, $4);
	area = area + $2;
	pop = pop + $3;
}

END {
	printf ("\n%10s %6d %5d\n", "TOTAL", area, pop);
}
