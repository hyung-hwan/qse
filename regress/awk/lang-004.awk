# A function and a named variable cannot have the same name.
function a () { }
BEGIN { a = 20; }
