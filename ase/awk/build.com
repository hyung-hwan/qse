$
$! build script for OpenVMS
$! define xp [dir.of.xpkit.xp]
$
$ names :=  awk,err,tree,tab,map,parse,run,sa,val,func,misc,extio,rex
$ gosub compile
$ gosub archive
$ exit
$
$ compile:
$   num = 0
$ compile_loop:
$   name = f$element(num,",",names)
$   if name .eqs. "," then return
$   gosub compile_file
$   num = num + 1
$   goto compile_loop
$
$ compile_file:
$   write sys$output "Compiling ''name'.c..."
$!   cc/define=XP_AWK_STAND_ALONE 'name'
$   cc/define=XP_AWK_STAND_ALONE /pointer_size=long 'name'
$   return
$
$ archive:
$   write sys$output "Creating library..."
$   lib/create xpawk 'names'
