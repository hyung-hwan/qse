$2 > maxrate { maxrate = $2; maxemp = $1; }
END { print "highest hourly rage:", maxrate, "for", maxemp; }
