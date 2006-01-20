
$ write sys$output "cc/define=__STAND_ALONE awk.c"
$ cc/define=__STAND_ALONE awk.c

$ write sys$output "link awk.obj,[-.-.awk]xpawk/library"
$ link awk.obj,[-.-.awk]xpawk/library
