BEGIN {
	split ("a b c d e", ARGV);
	for (i in ARGV) print i, ARGV[i];
}
