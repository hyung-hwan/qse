BEGIN  {
	CONVFMT="%s";
	#CONVFMT="%*.*s";
	#CONVFMT="%*.*f";
	printf "%s", sprintf ("abc %s abc\n", sprintf ("def %s %s", sprintf ("%s %s %s", "xyz", 1.2342, "xyz"), "def"));
}

