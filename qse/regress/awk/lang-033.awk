BEGIN {
	while ("cat /etc/passwd" | getline x > 0) 
		print x
}
