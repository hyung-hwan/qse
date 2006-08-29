BEGIN { /*RS = "Asia";*/ /*RS=746;*/ RS=/USA/; }
{ print "RECORD: ", $0; }
