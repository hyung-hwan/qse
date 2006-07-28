/hello/ 
{ 
	//print FILENAME; 
	print "**1**" $0; 
	//nextfile;
	print "----------------";
}

/hello/ 
{ 
	//print FILENAME; 
	print "**2**" $0; 
	nextfile;
	print "----------------";
}

END
{
	print "== END OF PROGRAM ==";
}
