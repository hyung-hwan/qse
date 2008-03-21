function fn () { a = 20; return a;}
global a;
BEGIN { a = 30; print fn (); print a; }

