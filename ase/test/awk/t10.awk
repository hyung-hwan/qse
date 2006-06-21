BEGIN 
{ 
	getline x < "abc";  /* open("abc", O_RDONLY|O_LARGEFILE)       = 3 */
	//print 10 >> "abc";  /* open("abc", O_WRONLY|O_APPEND|O_CREAT|O_LARGEFILE, 0666) = 4 */
	getline x < "abc"; 
	//print x;  
	a = close ("abc");      /* close(4) */
	//print "hey"
	b = close ("abc");      /* close(3) */

	getline x < "Makefile.cl";
	getline y < "awk.c";
	c = close ("Makefile.cl");
	d = close ("awk.c");

}
