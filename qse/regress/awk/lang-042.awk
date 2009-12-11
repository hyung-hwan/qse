BEGIN { 
	print //; 
	print /=/; 
	print /.*/;

	a=5; a /= 10; print a;

	for (IGNORECASE=0; IGNORECASE<=1;IGNORECASE++)
	{
		print "IGNORECASE=", IGNORECASE;
		print "abc" ~ /^[[:upper:]]+$/; 
		print "abc" ~ /^[[:lower:]]+$/; 
		print "ABC" ~ /^[[:upper:]]+$/; 
		print "ABC" ~ /^[[:lower:]]+$/; 
		print "AbC" ~ /^[[:upper:]]+$/; 
		print "aBc" ~ /^[[:lower:]]+$/; 
	}
}
