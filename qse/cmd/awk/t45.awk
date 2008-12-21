BEGIN { t[1] = "abc"; gsub ("abc", "[&]", t); print t[1]; }
