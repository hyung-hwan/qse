#BEGIN { t = "abc"; gsub ("abc", "[&]", t); print t; }
{ c=$0; print sub ("abc", "ABC", c); print c; }
{ sub (/ABC/, "XYZ", c); print c; }
