BEGIN { IGNORECASE=1; }
$0 == "abc" {
	print "[" $0 "]";
}
