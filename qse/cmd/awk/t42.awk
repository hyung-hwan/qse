
BEGIN {
	FS=":";
	OFS="::";
}

{
	$2=1.23;
	NF=4;
	print "NF=" NF;
	print "[" $10 "]";
	print "$0=[" $0 "]";
}
