BEGIN 
{
	print "line 1" >> "1";
	print "line 2" > "1";
	print "line 3" >> "1";

	print "line 4" >> "2";
	print "line 4" >> "3";
	print "line 4" >> "4";

	while ((getline x < "abc") > 0) print x;
	close ("abc");


	print "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~";

	getline x < "abc";
	if (x == "a") print "xxxxxxxxxxxxxxxx"; else print x;
	
}
