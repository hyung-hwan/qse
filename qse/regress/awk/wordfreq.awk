# wordfreq.awk --- print list of word frequencies
     
{
	$0 = tolower($0);    # remove case distinctions

	# remove punctuation
	a=0;
	gsub(/[^[:alnum:]_[:blank:]]/, " ", $a);
	#gsub(/[^[:alnum:]_[:blank:]]/, " ");

	for (i = 1; i <= NF; i++) freq[$i]++;
}
  
END {
	for (word in freq)
		print word, freq[word];
}
