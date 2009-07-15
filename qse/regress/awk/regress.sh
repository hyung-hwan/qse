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

QSEAWK=${QSEAWK:=../../cmd/awk/qseawk}
TMPFILE="${TMPFILE:=./regress.temp}"
OUTFILE="${OUTFILE:=./regress.out}"

PROGS="
	cou-001.awk/cou.dat//
	cou-002.awk/cou.dat//
	cou-003.awk/cou.dat//
	cou-004.awk/cou.dat//
	cou-005.awk/cou.dat//
	cou-006.awk/cou.dat//
	cou-007.awk/cou.dat//
	cou-008.awk/cou.dat//
	cou-009.awk/cou.dat//
	cou-010.awk/cou.dat//
	cou-011.awk/cou.dat//
	cou-012.awk/cou.dat//
	cou-013.awk/cou.dat//
	cou-014.awk/cou.dat//
	cou-015.awk/cou.dat//
	cou-016.awk/cou.dat//
	cou-017.awk/cou.dat//
	cou-018.awk/cou.dat//
	cou-019.awk/cou.dat//
	cou-020.awk/cou.dat//
	cou-021.awk/cou.dat//
	cou-022.awk/cou.dat//
	cou-023.awk/cou.dat//
	cou-024.awk/cou.dat//
	cou-025.awk/cou.dat//
	cou-026.awk/cou.dat//
	cou-027.awk/cou.dat//

	emp-001.awk/emp.dat//
	emp-002.awk/emp.dat//
	emp-003.awk/emp.dat//
	emp-004.awk/emp.dat//
	emp-005.awk/emp.dat//
	emp-006.awk/emp.dat//
	emp-007.awk/emp.dat//
	emp-008.awk/emp.dat//
	emp-009.awk/emp.dat//
	emp-010.awk/emp.dat//
	emp-011.awk/emp.dat//
	emp-012.awk/emp.dat//
	emp-013.awk/emp.dat//
	emp-014.awk/emp.dat//
	emp-015.awk/emp.dat//
	emp-016.awk/emp.dat//
	emp-017.awk/emp.dat//
	emp-018.awk/emp.dat//
	emp-019.awk/emp.dat//
	emp-020.awk/emp.dat//
	emp-021.awk/emp.dat//
	emp-022.awk/emp.dat//
	emp-023.awk/emp.dat//
	emp-024.awk/emp.dat//
	emp-025.awk/emp.dat//
	emp-026.awk/emp.dat//
	emp-027.awk/emp.dat//

	adr-001.awk/adr.dat//
	adr-002.awk/adr.dat//

	unr-001.awk/unr.dat//

	lang-001.awk///--strictnaming=off --newline=on -o-
	lang-002.awk///--newline=on -o-
	lang-003.awk///--newline=on -o-
	lang-004.awk///--newline=on -o-
	lang-005.awk///--implicit=off --explicit=on --newline=on -o-
	lang-006.awk///--implicit=off --explicit=on --newline=on -o-
	lang-007.awk///--implicit=on --explicit=on --newline=on -o-
	lang-008.awk///--implicit=off --explicit=on --newline=on -o-
	lang-009.awk/lang-009.awk//--implicit=off --explicit=on --newline=on --strictnaming=off -o-
	lang-010.awk/this is just a test//--newline=on -o-
	lang-011.awk///--newline=on -o-
	lang-012.awk///--newline=on -o-
	lang-013.awk///--newline=on -o-
	lang-014.awk///--newline=on -o-
	lang-015.awk///--newline=on -o-
	lang-016.awk///--newline=on -o-
	lang-017.awk///--newline=on -o-
	lang-017.awk///--call main --newline=on -o-
	lang-018.awk///--explicit=on --newline=on -o-
	lang-019.awk///--explicit=on --newline=on -o-
	lang-020.awk///--explicit=on --newline=on -o-
	lang-021.awk///--explicit=on --newline=on -o-
	lang-022.awk///--newline=on -o-
	lang-023.awk///--explicit=on --newline=on -o-
	lang-024.awk///--explicit=on --newline=on -o-
	lang-025.awk///--newline=on -o-
	lang-026.awk///--newline=on -o-
	lang-027.awk///--newline=on -o-
	lang-028.awk///--newline=on -o-
	lang-029.awk///--explicit=on --newline=on -o-
	lang-030.awk///--newline=on -o-
	lang-031.awk///--newline=on -o-
	lang-032.awk///--newline=on -o-
	lang-033.awk///--newline=on -o-

	quicksort.awk/quicksort.dat//
	quicksort2.awk/quicksort2.dat//
	asm.awk/asm.s/asm.dat/
	stripcomment.awk/stripcomment.dat//
	wordfreq.awk/wordfreq.awk//
	hanoi.awk///
	indent.awk/indent.dat//
"

[ -x "${QSEAWK}" ] || 
{
	echo "ERROR: ${QSEAWK} not found"
	exit 1;
}

run_scripts() 
{
	echo "${PROGS}" > "${TMPFILE}"
	
	while read prog
	do
		[ -z "${prog}" ] && continue
	
		script="`echo ${prog} | cut -d/ -f1`"
		datafile="`echo ${prog} | cut -d/ -f2`"
		redinfile="`echo ${prog} | cut -d/ -f3`"
		awkopts="`echo ${prog} | cut -d/ -f4`"
	
		[ -z "${script}" ] && continue
		[ -f "${script}" ] || 
		{
			echo_so "${script} not found"
			continue
		}
	
		[ -z "${redinfile}" ] && redinfile="/dev/stdin"
	
		echo_title "${QSEAWK} ${awkopts} -f ${script} ${datafile} <${redinfile} 2>&1"
		${QSEAWK} ${awkopts} -f ${script} ${datafile} <${redinfile} 2>&1
	
	done < "${TMPFILE}" 
	
	rm -f "${TMPFILE}"
}

case $1 in
init)
	run_scripts > "${OUTFILE}"
	echo_so "INIT OK"
	;;
test)
	run_scripts > "${OUTFILE}.temp"
	diff -q "${OUTFILE}" "${OUTFILE}.temp" || {
		echo_so "ERROR: ${OUTFILE} differs from ${OUTFILE}.temp."
		echo_so "       Check the scripts and output files for any errors."
		exit 1
	}
	rm -f "${OUTFILE}.temp"
	echo_so "TEST OK"
	;;
*)
	echo_so "USAGE: $0 init"
	echo_so "       $0 test"
	exit 1
	;;
esac
	
exit 0
