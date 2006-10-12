BEGIN {
	print "ARGC=", ARGC;
	for (i in ARGV)
	{
		print "ARGV[" i "]", ARGV[i];
	}

	print "----------------------";
	print "ARGC=", ARGC;
	split ("111 22 333 555 666 777", ARGV);
	for (i in ARGV)
	{
		print "ARGV[" i "]", ARGV[i];
	}


	if (ARGC >= 0) print "ARGC is positive";
}
