#
# ARGC points to a scalar value.
# split() can turn it into a map if FLEXMAP is on.
# it is an error if FLEXMAP is off.
#
BEGIN {
	split ("a b c d e", ARGC);
	for (i in ARGC) print i, ARGC[i];
}
