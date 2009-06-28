function f(x)
{
	print x;
	f("my hello");
}

BEGIN { f(10); }
