BEGIN {
	print 1 == 1;
	print 1 == 0;

	print 1.0 == 1;
	print 1.1 == 1;

	print 1.0 != 1;
	print 1.1 != 1;

	print "abc" == "abc";
	print "abc" != "abc";

	a[10] = 2;
	print a == 1;
}
