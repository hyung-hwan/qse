# cannot use function name as a parameter name 
# unless QSE_AWK_STRICTNAMING is off
function f(f) { print f; }

/* 
 * the begin block 
 */
BEGIN { f("hello"); }
