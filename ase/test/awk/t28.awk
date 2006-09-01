#global xyz;

END {
	#local xyz;

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

	/*
	a[1] = 20;
	substr (a, 3, 4);
	*/

	print tolower ("AbcDEF");
	print toupper ("AbcDEF");

	arr[0] = "xxx";
	#print split ("abc def abc", arr);
	print split ("abc def kkk", j);

	#xyz = 20;
	#print xyz;
	print split ("abc def kkk", ((xyz)));
	#for (i in arr)

	for (i in xyz)
	{
		print i, " ", xyz[i];
	}
}
