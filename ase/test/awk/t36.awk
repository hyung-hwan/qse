# test cases
#
# input data []
# result:
#  NF=0
#
# input data [abcdefg]
#  NF=2
#  0 []
#  1 [bcdefg]
# 
# input data [abdefg   abcdefg]
#  NF=3
#  0 []
#  1 [bdefg   ]
#  2 [bcdefg]
#
# input data [   abcdefg hij  a a]
#  NF=4
#  0 [   ]
#  1 [bcdefg hij  ]
#  2 [ ]
#  3 []
#
# input data [   abcdefg hij  a a  ]
#  NF=4
#  0 [   ]
#  1 [bcdefg hij  ]
#  2 [ ]
#  3 [  ]
#
# input data [aaaaa]
#  NF=6
#  0 []
#  1 []
#  2 []
#  3 []
#  4 []
#  5 []
#

BEGIN { FS="a"; } 
{
	print "NF=" NF; 
	for (i = 0; i < NF; i++) print i " [" $(i+1) "]";
}

