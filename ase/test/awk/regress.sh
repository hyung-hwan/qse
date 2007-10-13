#!/bin/sh

OPTION=-explicit

run_script_for_init()
{
	script="$1"
	data="$2"
	output=`echo $script | sed 's/\.awk$/.out/g'`

	"$ASEAWK" $OPTION -d -f "$script" "$data" > "$output"
}

run_init()
{
	for script in emp-???.awk
	do
		run_script_for_init "$script" "emp-en.data"
	done

	for script in cou-???.awk
	do
		run_script_for_init "$script" "cou-en.data"
	done

	for script in adr-???.awk
	do
		run_script_for_init "$script" "adr-en.data"
	done

	for script in err-???.awk
	do
		run_script_for_init "$script" "err-en.data"
	done
}

run_script_for_test()
{
	script="$1"
	data="$2"
	output=`echo $script | sed 's/\.awk$/.out/g'`

	echo ">> RUNNING $script"
	"$ASEAWK" $OPTION -d -f "$script" "$data" > "$output.$pid"

	#diff -y "$output" "$output.$pid" 
	diff "$output" "$output.$pid" 
	if [ $? -ne 0 ]
	then
		rm -f "$output.$pid"
		return 1
	fi

	rm -f "$output.$pid"
	return 0
}

run_test()
{
	pid=$$

	for script in emp-???.awk
	do
		run_script_for_test "$script" "emp-en.data"
		if [ $? -ne 0 ]
		then
			echo "###################################"
			echo "PROBLEM(S) DETECTED IN $script.".
			echo "###################################"

			echo "Do you want to abort? [y/n]"
			read ans
			if [ "$ans" = "y" -o "$ans" = "Y" ]
			then
				return 1
			fi
		fi
	done

	for script in cou-???.awk
	do
		run_script_for_test "$script" "cou-en.data"
		if [ $? -ne 0 ]
		then
			echo "###################################"
			echo "PROBLEM(S) DETECTED IN $script.".
			echo "###################################"

			echo "Do you want to abort? [y/n]"
			read ans
			if [ "$ans" = "y" -o "$ans" = "Y" ]
			then
				return 1
			fi
		fi
	done

	for script in adr-???.awk
	do
		run_script_for_test "$script" "adr-en.data"
		if [ $? -ne 0 ]
		then
			echo "###################################"
			echo "PROBLEM(S) DETECTED IN $script.".
			echo "###################################"

			echo "Do you want to abort? [y/n]"
			read ans
			if [ "$ans" = "y" -o "$ans" = "Y" ]
			then
				return 1
			fi
		fi
	done

	for script in err-???.awk
	do
		run_script_for_test "$script" "err-en.data"
		if [ $? -ne 0 ]
		then
			echo "###################################"
			echo "PROBLEM(S) DETECTED IN $script.".
			echo "###################################"

			echo "Do you want to abort? [y/n]"
			read ans
			if [ "$ans" = "y" -o "$ans" = "Y" ]
			then
				return 1
			fi
		fi
	done

	return 0
}

#--------#
#  main  #
#--------#

if [ -x ./aseawk ]
then
	ASEAWK="./aseawk"
elif [ -x ../../release/bin/aseawk ]
then
	ASEAWK="../../release/bin/aseawk"
elif [ -x ../../debug/bin/aseawk ]
then
	ASEAWK="../../debug/bin/aseawk"
else
	echo "Error: cannot locate a relevant awk interpreter"
	exit 1;
fi

if [ $# -ne 1 ]
then
	echo "Usage: $0 init"
	echo "       $0 test"
	exit 1
fi

if [ "$1" = "init" ]
then
	run_init	
elif [ "$1" = "test" ]
then
	run_test
else
	echo "Usage: $0 init"
	echo "       $0 test"
	exit 1
fi

