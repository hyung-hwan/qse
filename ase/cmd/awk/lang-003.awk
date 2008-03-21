# should print 50
function fn(f) { f = 20; }
BEGIN { f = 50; fn(100); print f; }
