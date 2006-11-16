BEGIN  {
	CONVFMT="%s";
	printf "%s", sprintf ("abc %s abc\n", sprintf ("def %s %s", sprintf ("xyz %s xyz", 1.2342), "def"));
}

