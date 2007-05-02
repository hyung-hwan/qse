function a()
{
	print "aaaa";
	a();
}

BEGIN { 
	a = (b = 20); 
	print a; print b; for(i=j=1; i< 10; i++) print i, j; 

	a += b += 20; 
	print a; print b; for(i=j=1; i< 10; i++) print i, j; 

	j = (a < 20)? k = 20: c = 30;
	print (a < 20)? k = 20: c = 30;
	print "j=" j;
	print "k=" k;
	print "c=" c;

	a();
}


