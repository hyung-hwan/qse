/*global x, j;*/

func func1 (x)
{
	c = x["abc"];
	x["abc"] = 123;
}

BEGIN
{
	idx="abc";
	x[idx] = 12345;
	i = x[idx];
	x["1"] = 1;
	x["2"] = 2;

	func1 (x);
	k = x[idx];
	return j;

	/*
	x["abc"] = 12345;
	i = x["abc"];
	return j;
	*/
}
