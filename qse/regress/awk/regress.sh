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

QSEAWK="../../cmd/awk/qseawk"
PROGS="
	emp-001.awk/emp.d//
	emp-002.awk/emp.d//
	emp-003.awk/emp.d//
	emp-004.awk/emp.d//
	emp-005.awk/emp.d//
	emp-006.awk/emp.d//
	emp-007.awk/emp.d//
	emp-008.awk/emp.d//
	emp-009.awk/emp.d//
	emp-010.awk/emp.d//
	emp-011.awk/emp.d//
	emp-012.awk/emp.d//
	emp-013.awk/emp.d//
	emp-014.awk/emp.d//
	emp-015.awk/emp.d//
	emp-016.awk/emp.d//
	emp-017.awk/emp.d//
	emp-018.awk/emp.d//
	emp-019.awk/emp.d//
	emp-020.awk/emp.d//
	emp-021.awk/emp.d//
	emp-022.awk/emp.d//
	emp-023.awk/emp.d//
	emp-024.awk/emp.d//
	emp-025.awk/emp.d//
	emp-026.awk/emp.d//
	emp-027.awk/emp.d//

	quicksort.awk/quicksort.d//
	quicksort2.awk/quicksort2.d//
	asm.awk/asm.d1/asm.d2/
	stripcomment.awk/stripcomment.d//
	wordfreq.awk/wordfreq.awk//
	hanoi.awk//
"

[ -x "${QSEAWK}" ] || {
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
