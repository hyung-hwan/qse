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


	#for (i = 0
	#     i < 20
	#     i;;) print i;


	if (ARGC >= 0) printf "ARGC is positive";
}
