BEGIN { /*RS = "Asia";*/ /*RS=746;*/ /*RS="";*/ RS=/USA/; }
{ print NR, " ", $0; }
