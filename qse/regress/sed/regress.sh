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
		echo -n "[CMD] $1 "
		echo -n "[CMD] $1 " >/dev/stderr
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

QSESED_BASENAME="`basename "${QSESED}"`"

TMPFILE="${TMPFILE:=./regress.temp}"
OUTFILE="${OUTFILE:=./regress.out}"
OUTFILE_XMA="${OUTFILE}.xma"
XMAOPTS="-m 500000"

PROGS="
	s001.sed/s001.dat//-n
	s002.sed/s002.dat//
	s003.sed/s003.dat//
	s004.sed/s004.dat//
"

[ -x "${QSESED}" ] || 
{
	echo "ERROR: ${QSESED} not found"
	exit 1;
}

run_scripts() 
{
	valgrind="$1"
	extraopts="$2"
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

		echo_title "${valgrind} ${QSESED_BASENAME} ${extraopts} ${options} -f ${script} ${datafile} <${redinfile} 2>&1"
		${valgrind} ${QSESED} ${extraopts} ${options} -f ${script} ${datafile} <${redinfile} 2>&1
	
	done < "${TMPFILE}" 
	
	rm -f "${TMPFILE}"
}

run_test()
{
	outfile="${1}"
	extraopts="${2}"

	run_scripts "" "${extraopts}"> "${outfile}.test"

	# diff -q is not supported on old platforms.
	# redirect output to /dev/null instead.
	diff "${outfile}" "${outfile}.test" > /dev/null || {
		echo_so "ERROR: Difference is found between expected output and actual output."
		echo_so "       The expected output is stored in '${outfile}'."
		echo_so "       The actual output is stored in '${outfile}.test'."
		echo_so "       You may execute 'diff ${outfile} ${outfile}.test' for more info."
		return 1
	}
	rm -f "${outfile}.test"
	return 0
}

case $1 in
init)
	run_scripts "" "" > "${OUTFILE}"
	run_scripts "" "${XMAOPTS}" > "${OUTFILE_XMA}"
	echo_so "INIT OK"
	;;
test)
	run_test "${OUTFILE}" "" && {
		run_test "${OUTFILE_XMA}" "${XMAOPTS}" && {
			${QSESED} "s|${QSESED_BASENAME} ${XMAOPTS}|${QSESED_BASENAME} |" "${OUTFILE_XMA}" > "${OUTFILE_XMA}.$$"
			diff "${OUTFILE}" "${OUTFILE_XMA}.$$"  || {
				rm -f "${OUTFILE_XMA}.$$"
				echo_so "ERROR: Difference is found between normal output and xma output."
				echo_so "       The normal output is stored in '${OUTFILE}'."
				echo_so "       The xma output is stored in '${OUTFILE_XMA}'."
				echo_so "       Ignore lines staring with [CMD] in the difference."
				exit 1;
			}
			rm -f "${OUTFILE_XMA}.$$"
			echo_so "TEST OK"
		}
	}
	;;
leakcheck)
	bin_valgrind="`which valgrind 2> /dev/null || echo ""`"
	[ -n "${bin_valgrind}" -a -f "${bin_valgrind}" ] || {
		echo_so "valgrind not found. cannot perform this test"
		exit 1
	}
	run_scripts "${bin_valgrind} --leak-check=full --show-reachable=yes --track-fds=yes" 2>&1 > "${OUTFILE}.test"
	x=`grep -Fic "no leaks are possible" "${OUTFILE}.test"`
	y=`grep -Fic "${bin_valgrind}" "${OUTFILE}.test"`
	if [ ${x} -eq ${y} ] 
	then
		echo_so "(POSSIBLY) no memory leaks detected".
	else
		echo_so "(POSSIBLY) some memory leaks detected".
	fi
	echo_so "Inspect the '${OUTFILE}.test' file for details"
	;;
*)
	echo_so "USAGE: $0 init"
	echo_so "       $0 test"
	echo_so "       $0 leakcheck"
	exit 1
	;;
esac
	
exit 0
