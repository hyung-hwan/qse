BEGIN {
	RS="[\t\n\v\f\r ]*[\r\n]+[\t\n\v\f\r ]*"
} 

{
	print $0
}
