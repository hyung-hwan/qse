BEGIN {
	/*
	print "line 1" >> "1";
	print "line 2" > "1";
	print "line 3" >> "1";

	print "line 4" >> "2";
	print "line 4" >> "3";
	print "line 4" >> "4";

	while ((getline x < "abc") > 0) print x;
	close ("abc");


	print "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~";

	getline x < "abc";
	if (x == "a") print "xxxxxxxxxxxxxxxx"; else print x;
	*/

	/*
	print getline x;
	print "[", x, "]";
	print "--------------";
	*/

	if (getline x < "abc" == -1)
	{
		print "ERRNO = ", ERRNO;
	}

	print x > "def";
	#print "fflush(def) = ", fflush ("def");
	print "fflush() = ", fflush ("");

#	print (1 (2 getline j) j);
#	print "abc" 1 + 2 3 + 49 2 / 3;
}
