//"abc" != "def"  {
/a\/b/ {
	print $0 ~ /abc/;
	print $0 !~ /abc/;
	print $0 ~ "abc[[:space]]";
	print $0 !~ "abc";
	print /abc/;
}
