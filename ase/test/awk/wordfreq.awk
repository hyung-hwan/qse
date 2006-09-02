# wordfreq.awk --- print list of word frequencies
     
{
	$0 = tolower($0);    # remove case distinctions
	# remove punctuation
	#gsub(/[^[:alnum:]_[:blank:]]/, "", $0);

	for (i = 1; i <= NF; i++) freq[$i]++;
}
     
END {
	for (word in freq)
		print word, "\t", freq[word];
}
     

