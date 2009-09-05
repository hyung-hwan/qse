{ 
	if ($0 ~ /^-+$/) { 
		getline x; printf " %s\n", x; nobar=0;
	} 
	else { 
		if (nobar) printf "\n"; printf "%s", $0; nobar=1; 
	}
}
