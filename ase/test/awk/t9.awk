BEGIN 
{
	while (("xxx /p" | getline var) > 0) ;
	while (("dir /w" | getline var) > 0) ;
	while ((getline var < "t9.awk") > 0) ;

	zzz = close ("xxx /p");
	/*
	while ("ls -l" | getline var) 
	{ 
		"ls -l" | getline x; 
		print var; print x;
	} 

	while (getline < "/etc/passwd")
	{
		print $0;
	}

	while (getline x < "/etc/shadow")
	{
		print x;
	}
	*/
}

