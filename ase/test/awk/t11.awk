BEGIN
{
	print "this is only a test";
	print;
	print 1, 2, (3 >> 10);
	print 1, 2, 3 >> 10;
	print 3, 4, 5 >> 10;
	close (10);
	print "-------------" >> 10;

	delete abc;
	delete abc["aaaa"] ;

	/*
	print 1 > 2 + 3;
	print 1 < 2 + 3;
	*/
}
