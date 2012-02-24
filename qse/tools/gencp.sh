
#
# get the following unicode mapping files 
# from unicode.org before executing this script.
#   CP932.TXT CP936.TXT CP949.TXT CP950.TXT
#

gencp() {
	name="$1"
	max_gap="$2"

	qseawk -vMAX_GAP="${max_gap}"  --extraops=on -f gencp1.awk "`echo $name | tr '[a-z]' '[A-Z]'`.TXT" > "${name}.h" 2>/dev/null
	ln -sf "${name}.h" x.h
	cc -o testcp testcp.c
	qseawk --extraops=on -f gencp0.awk "`echo $name | tr '[a-z]' '[A-Z]'`.TXT" > "${name}.0" 2>/dev/null
	./testcp > "${name}.1" 
	diff -q "${name}.0" "${name}.1" && echo "[$name] OK" || echo "[$name] NOT OK"
}

gencp cp932 64  # ms shift-jis
gencp cp936 96  # ms gbk
gencp cp949 128 # ms euc-kr
gencp cp950 64  # ms big5
