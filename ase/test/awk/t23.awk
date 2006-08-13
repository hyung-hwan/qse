/*
{
	print "ALL ==> " $0;
}

/1/,/5/
{

	print "/1/,/5/ ==> " $0;
}
*/


BEGIN { c["Europe"] = "XXX"; }

/Europe/, /Africa/ { print $0;  }

//(a = "20") { }
/*"Europe" in c { print $0; }*/
