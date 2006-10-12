BEGIN { 
	FS="[a-c]+";
	IGNORECASE=0.1;
} 
{
	print "NF=" NF; 
	for (i = 0; i < NF; i++) print i " [" $(i+1) "]";
}
