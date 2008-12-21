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
	#     i;;) print "[" i "]";
	#for (i = 0
	#     (i < 20)
	#     i;;) print "[" i "]";

	#printf 10, 20, 30;
	if (ARGC >= 0) printf ("ARGC [%++#10.10i] is positive\n", 10);
	if (ARGC >= 0) printf ("ARGC [%++#10.10f] is positive\n", 10);
	if (ARGC >= 0) printf ("ARGC [%++#10.10E] is positive\n", 10124.1123);
	if (ARGC >= 0) printf ("ARGC [%++#10.10G] is positive\n", 10124.1123);
	if (ARGC >= 0) printf ("ARGC [%++#10.10g] is positive\n", 10124.1123);
	if (ARGC >= 0) printf ("ARGC [%++#10.10f] is positive\n", 10124.1123);
	printf ("[%d], [%f], [%s]\n", 10124.1123, 10124.1123, 10124.1123);
	printf ("[%-10c] [% 0*.*d]\n", 65, 45, 48, -1);

	print sprintf ("abc%d %*.*d %c %s %c", 10, 20, 30, 40, "good", "good", 75.34);
}
