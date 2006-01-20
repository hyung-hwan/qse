
$ write sys$output "cc/define=__STAND_ALONE awk.c"
$ cc/define=__STAND_ALONE awk.c

$ write sys$output "cc/define=__STAND_ALONE parse.c"
$ cc/define=__STAND_ALONE parse.c

$ write sys$output "cc/define=__STAND_ALONE tree.c"
$ cc/define=__STAND_ALONE tree.c

$ write sys$output "cc/define=__STAND_ALONE sa.c"
$ cc/define=__STAND_ALONE sa.c


$ write sys$output "lib/create xpawk awk,parse,tree,sa"
$ lib/create xpawk awk,parse,tree,sa
