#{ "dir/w/p" | getline;  print $0; print "flush(dir/w/p)=", fflush("dir/w/p"); }
#{ print | "grep Asia"; fflush("grep Asia"); }
{ print | "grep Asia"; print "flush(grep Asia)=", fflush("grep Asia"); }
