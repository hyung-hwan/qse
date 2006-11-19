#!/bin/sh

pid=$$

for script in emp???.awk
do
	output=`echo $script | sed 's/\.awk$/.out/g'`
	./awk $script emp-en.data > "$output.$pid"

	diff $output "$output.$pid" 
	if [ $? -ne 0 ]
	then
		echo "###################################"
		echo "PROBLEM(S) DETECTED IN $script.".
		echo "###################################"
		rm -f "$output.$pid"
		break
	fi

	rm -f "$output.$pid"
done
