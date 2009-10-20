BEGIN { 
	print length 11  # this should print 011 as length is length($0);
	print length (11) # this should print 2 
} 
