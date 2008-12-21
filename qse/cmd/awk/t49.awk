BEGIN { OFS=":"; }
{
	print $5 = "abc";
	print $0;
}
