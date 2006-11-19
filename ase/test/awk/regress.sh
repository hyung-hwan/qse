#!/bin/sh

init()
{
	for script in emp-???.awk
	do
		output=`echo $script | sed 's/\.awk$/.out/g'`
		./awk $script emp-en.data > "$output"
	done
}

test()
{
	pid=$$

	for script in emp-???.awk
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
}

#--------#
#  main  #
#--------#

if [ $# -ne 1 ]
then
	echo "Usage: $0 init"
	echo "       $0 test"
	exit 1
fi

if [ "$1" = "init" ]
then
	init	
elif [ "$1" = "test" ]
then
	test
else
	echo "Usage: $0 init"
	echo "       $0 test"
	exit 1
fi

