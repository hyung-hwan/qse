BEGIN {
	a[1]=1;
	a[2]=2;
	b[4]=4;
	b[5]=5;

	#for (i in a) delete a[i];
	#for (i in b) a[i]=b[i];
	# if FLEXMAP is on, a = b acts like the 2 lines commented out above.
	a = b;
	for (i in a) print i, a[i];
}
