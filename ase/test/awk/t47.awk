BEGIN {
	
	a[1] = 20;
	a[2] = 40;
	a[3,5,6] = 40;
	a["abc"] = 20;

	for (i in a) print "a[" i "]=" a[i];
	#SUBSEP=",,,";
	#SUBSEP=4.5;
	SUBSEP=555;

	print "------------------------";

	a[9,x,3] = 40;
	for (i in a) print "a[" i "]=" a[i];
}


