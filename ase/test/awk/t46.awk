BEGIN { t = "whoabcabcabcwho"; gsub ("abc", "[\\&][&][\\&]", t); print t; }
