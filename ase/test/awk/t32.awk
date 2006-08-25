BEGIN { RS = ""; }
{ print "RECORD: ", $0; }
