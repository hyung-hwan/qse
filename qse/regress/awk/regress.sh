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

print_usage()
{
	echo "Usage: $0 init"
	echo "       $0 test"
}

###################
# MAIN            #
###################

QSEAWK=${QSEAWK:=../../cmd/awk/qseawk}

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

	quicksort.awk/quicksort.dat//
	quicksort2.awk/quicksort2.dat//
	asm.awk/asm.s/asm.dat/
	stripcomment.awk/stripcomment.dat//
	wordfreq.awk/wordfreq.awk//
	hanoi.awk//
	indent.awk/regress.sh/
"

[ -x "${QSEAWK}" ] || 
{
	echo "ERROR: ${QSEAWK} not found"
	exit 1;
}

for prog in ${PROGS}
do
	script="`echo ${prog} | cut -d/ -f1`"
	datafile="`echo ${prog} | cut -d/ -f2`"
	redinfile="`echo ${prog} | cut -d/ -f3`"
	awkopts="`echo ${prog} | cut -d/ -f4`"

	if [ -n "${redinfile}" ]
	then
		echo_so "${QSEAWK} ${awkopts} -f ${script} ${datafile} < ${redinfile}"
		${QSEAWK} ${awkopts} -f ${script} ${datafile} < ${redinfile}
	else
		echo_so "${QSEAWK} ${awkopts} -f ${script} ${datafile}"
		${QSEAWK} ${awkopts} -f ${script} ${datafile} 
	fi
done

exit 0
