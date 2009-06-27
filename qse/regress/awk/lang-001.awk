# cannot use function name as a parameter name
function f(f) { print f; }
BEGIN { f("hello"); }
