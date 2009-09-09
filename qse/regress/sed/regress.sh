#!/bin/sh

echo_so()
{
	tput smso
	while [ $# -gt 0 ]
	do
		echo -n "$1 "
		shift
	done		
	echo
	tput rmso
}

echo_title()
{
	echo "--------------------------------------------------------------------------------"
	while [ $# -gt 0 ]
	do
		echo -n "$1 "
		echo -n "$1 " >/dev/stderr
		shift
	done		
	echo
	echo > /dev/stderr
	echo "--------------------------------------------------------------------------------"
}

print_usage()
{
	echo_so "Usage: $0 init"
	echo_so "       $0 test"
}

###################
# MAIN            #
###################

[ -z "${QSESED}" ] && {
	QSESED=../../cmd/sed/.libs/qsesed
	[ -f "${QSESED}" ] || QSESED=../../cmd/sed/qsesed
}
[ -f "${QSESED}" -a -x "${QSESED}" ] || {
	echo_so "the executable '${QSESED}' is not found or not executable"
	exit 1
}

TMPFILE="${TMPFILE:=./regress.temp}"
OUTFILE="${OUTFILE:=./regress.out}"

PROGS="
	001.sed/001.dat//-n
	002.sed/002.dat//
	003.sed/003.dat//
	004.sed/004.dat//
"

[ -x "${QSESED}" ] || 
{
	echo "ERROR: ${QSESED} not found"
	exit 1;
}

run_scripts() 
{
	valgrind="$1"
	echo "${PROGS}" > "${TMPFILE}"
	
	while read prog
	do
		[ -z "${prog}" ] && continue
	
		script="`echo ${prog} | cut -d/ -f1`"
		datafile="`echo ${prog} | cut -d/ -f2`"
		redinfile="`echo ${prog} | cut -d/ -f3`"
		options="`echo ${prog} | cut -d/ -f4`"
	
		[ -z "${script}" ] && continue

		[ -f "${script}" ] || 
		{
			echo_so "${script} not found"
			continue
		}
	
		[ -z "${redinfile}" ] && redinfile="/dev/stdin"

		echo_title "${valgrind} ${QSESED} ${options} -f ${script} ${datafile} <${redinfile} 2>&1"
		${valgrind} ${QSESED} ${options} -f ${script} ${datafile} <${redinfile} 2>&1
	
	done < "${TMPFILE}" 
	
	rm -f "${TMPFILE}"
}

case $1 in
init)
	rm -f *.dp
	run_scripts > "${OUTFILE}"
	rm -f *.dp
	echo_so "INIT OK"
	;;
test)
	run_scripts > "${OUTFILE}.test"

	# diff -q is not supported on old platforms.
	# redirect output to /dev/null instead.
	diff "${OUTFILE}" "${OUTFILE}.test" > /dev/null || {
		echo_so "ERROR: Difference is found between expected output and actual output."
		echo_so "       The expected output is stored in '${OUTFILE}'."
		echo_so "       The actual output is stored in '${OUTFILE}.test'."
		echo_so "       You may execute 'diff ${OUTFILE} ${OUTFILE}.test' for more info."
		exit 1
	}
	rm -f "${OUTFILE}.test"
	echo_so "TEST OK"
	;;
leakcheck)
	bin_valgrind="`which valgrind 2> /dev/null || echo ""`"
	[ -z "${bin_valgrind}" ] && {
		echo_so "valgrind not found. cannot perform this test"
		exit 1
	}
	run_scripts "${bin_valgrind} --leak-check=full --show-reachable=yes --track-fds=yes" 2>&1 > "${OUTFILE}.test"
	echo_so "Inspect the '${OUTFILE}.test' file for any memory and file descriptor leaks."
	;;
*)
	echo_so "USAGE: $0 init"
	echo_so "       $0 test"
	echo_so "       $0 leakcheck"
	exit 1
	;;
esac
	
exit 0
