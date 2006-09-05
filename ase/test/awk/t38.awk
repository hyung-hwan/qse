
BEGIN {
	split (" a b c d e ", x, "");
	for (j in x) print j "->" x[j];
	print "-------------------";
	split ("a b c d e", x, "b c");
	for (j in x) print j "->" x[j];
	print "-------------------";
}
