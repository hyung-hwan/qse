BEGIN {

	a["abc\0\0xxx"] = "abcdefg";
	print a["abc"];
	print a["abc\0\0xxx"];
}
