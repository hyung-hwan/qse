BEGIN {
        print "15" || "sort";
        print "14" || "sort";
        print "13" || "sort";
        print "12" || "sort";
        print "11" || "sort";
        #close the input as sort emits when the input is closed
        close ("sort", "r");
        #close ("sort", "w");
	print "-----";
        while (("sort" || getline x) > 0) print "xx:", x;
}

