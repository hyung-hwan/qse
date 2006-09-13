#BEGIN { t = "abc"; gsub ("abc", "[&]", t); print t; }
{ c=$0; print gsub ("abc", "ABC", c); print c; }
{ gsub (/ABC/, "XYZ", c); print c; }
