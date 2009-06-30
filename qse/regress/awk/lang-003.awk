# a parameter can shade a named variable.
# it should print 50

BEGIN { f = 50; fn(100); print f; }
function fn(f) { f = 20; }
