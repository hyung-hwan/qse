BEGIN { 
	ARGV[1] = 20;
	print "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";

	delete NF;
	print "NF[1]=", NF[1];

	NF[1] = 20;  // this line should not be allowed
	print "AWK IMPLEMENTATION ERROR: hey... NF[1] = 20 has succeeded in the BEGIN block. your interpreter must be wrong";
	print "NF[1]=", NF[1];
}

{
	NF = 30; 
}
