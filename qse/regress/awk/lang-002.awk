function f(f)
{
	print f;
	f("my hello");
}

BEGIN { f(10); }
