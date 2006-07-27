/hello[[:space:]]/ 
{ 
	print $0; 
	//getline a;
	//print a;

	if (getline > 0) print $0;
	print "----------------";
}
