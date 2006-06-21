BEGIN 
{ 
	getline x < "abc";  /* open("abc", O_RDONLY|O_LARGEFILE)       = 3 */
	print 10 >> "abc";  /* open("abc", O_WRONLY|O_APPEND|O_CREAT|O_LARGEFILE, 0666) = 4 */
	getline x < "abc"; 
	print x;  
	close ("abc");      /* close(4) */
	print "hey"
	close ("abc");      /* close(3) */
}
