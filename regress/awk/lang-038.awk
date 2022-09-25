#
# test the third parameter(starting position) of index and match
#

BEGIN {
	xstr = "abcdefabcdefabcdef";
	xsub = "abc";
	xlen = length(xsub);

	i = 1;
	while ((i = index(xstr, xsub, i)) > 0)
	{
		print i, substr(xstr, i, xlen);
		i += xlen;
	}

	print "----------------";

	i = 1;
	while (match(xstr, xsub, i) > 0)
	{
		print RSTART, substr(xstr, RSTART, RLENGTH);
		i = RSTART + RLENGTH;
	}
}
