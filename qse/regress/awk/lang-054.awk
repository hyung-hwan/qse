# ARGV is an array. if FLEXMAP is off, it should trigger an error.
# If it is on, ARGV can change to a scalar value.
BEGIN {
	print getioattr ("xxxx", "rtimeout", ARGV);
	print ARGV;
}
