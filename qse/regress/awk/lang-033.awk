BEGIN {
	while ("cat lang-033.awk" | getline x > 0) 
		print x
}
