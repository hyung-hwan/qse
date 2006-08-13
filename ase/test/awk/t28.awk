END {
	print index ("abc", "abc");
	print index ("abc", "b");
	print index ("abc", "kabc");

	print "----------------------------";
	print substr ("abc", "abcdefg", 5);
	print substr ("abc", -1, 5);
	print substr ("abc", 0, 5);
	print substr ("abc", 1, 5);
	print substr ("abc", 2.829, 5);
	print substr ("abc", "3", 5);
	print substr ("abc", 4, 5);

	a[1] = 20;
	substr (a, 3, 4);
}
