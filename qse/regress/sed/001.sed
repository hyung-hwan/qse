# require -n
h
:again n
/^-+$/ {
        g
        N
        s/\n/ /
        p
        b skip
}
x
p
b again
:skip
