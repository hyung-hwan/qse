BEGIN {
	a=91
	print a ++10; # print 9110
	print a ++10; # print 9210
	print (a) ++10; # print 9310
	print ((a)) ++10; # print 9410
	print ((a)++) 10; # print 9510

	print "---------------------"
	a=91
	print (++(a)) 10; # print 9210
}


