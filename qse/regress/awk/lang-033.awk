BEGIN {
	while (("cat " datadir "/" datafile) | getline x > 0) 
		print x
}
