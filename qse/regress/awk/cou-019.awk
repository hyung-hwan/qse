function basename (str) { 
	ridx = str::rindex (str, "/");
	if (ridx == 0) return str;
	else return substr (str, ridx + 1);
}

FNR == 1, FNR == 5 { print basename(FILENAME) ": " $0; }
