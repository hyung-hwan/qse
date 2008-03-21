BEGIN  {
	#CONVFMT="%s";
	#CONVFMT="%*.*s";
	#CONVFMT="%*.*f";
	printf "[[[[[%s]]]]\n", sprintf ("abc %s abc", sprintf ("def %s %s", sprintf ("%s %s %s", "xyz", 1.2342, "xyz"), sprintf ("ttt %s tttt", 123.12)));

	printf "[[[[%s]]]]\n", sprintf ("ttt %s tttt", 123.12);
}

