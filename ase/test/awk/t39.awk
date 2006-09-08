BEGIN { 
	FS="[:[:space:]]+"; 
	#FS=":+"; 
} 
{
	print "NF=" NF; 
	for (i = 0; i < NF; i++) print i " [" $(i+1) "]";
}

