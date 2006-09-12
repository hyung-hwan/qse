/hello[[:space:]]/ { 
	print $0; 
	print "INTERNAL LOOP NF=" NF;
	for (i = 0; i < NF; i++)
	{
		print "[" $(i+1) "]";
	}
	#getline a;
	#print a;

	if (getline > 0) print $0;
	print "GETLINE NF=" NF;
	for (i = 0; i < NF; i++)
	{
		print "[" $(i+1) "]";
	}
	print "----------------";
}
