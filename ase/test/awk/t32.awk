BEGIN { RS = ""; }
{ print "record: ", $0; }
