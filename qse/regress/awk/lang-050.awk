# When a is assigned 20, a points to a scalar value.
# The second line turns it to a map if FLEXMAP is on.
# It is prohibited and results in an error if FLEXMAP is off.
BEGIN {
	a=20; 
	a[10]=30;
	for (i in a) print i, a[i];
}
