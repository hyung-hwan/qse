BEGIN 
{
	print "line 1" >> "1";
	print "line 2" > "1";
	print "line 3" >> "1";

	print "line 4" >> "2";
	print "line 4" >> "3";
	print "line 4" >> "4";
}
