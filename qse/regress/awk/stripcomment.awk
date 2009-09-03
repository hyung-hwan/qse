BEGIN {
	RS = "/\\*([^*]|\\*+[^/*])*\\*+/"
	# comment is record separator
	ORS = " "
	getline hold
}

{ print hold ; hold = $0 }

END { print hold }

