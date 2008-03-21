#BEGIN { t = "abc"; gsub ("abc", "[&]", t); print t; }
{ gsub ("abc", "ABC"); print $0; }
{ gsub (/ABC/, "XYZ"); print $0; }
