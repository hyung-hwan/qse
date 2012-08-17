BEGIN {
	print print print print 10;
	print (print 10 > "/tmp/should/not/be/creatable");
	if ((print 10 > "/tmp/should/not/be/creatable") <= -1)
		print "FAILURE";
	else 
		print "SUCCESS";

	print "------------------------------------";
	a = 1 (print 10 > "/tmp/should/not/be/creatable");
	print a;
	print "------------------------------------";
	b = ++a print 10;
	printf "%d\n",  b;
	print "------------------------------------";
	printf ("%d\n",  b + (((print print print 30 + 50)) + 40));
	print "------------------------------------";
	printf ("%d\n",  b + (((print print print 30  50)) + 40));

	print "------------------------------------";
	$(print 0 > "/dev/null") = "this is wonderful"; print $0;
	print "------------------------------------";
	$(getline dummy > "/dev/zero") = "that"; print $0;

	print "------------------------------------";
	x[0]=20; abc=(print ("hello", "world", (1, abc=20, abc=45))) in x; print abc
}


