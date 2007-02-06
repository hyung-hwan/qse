#
# OpenVMS MMS/MMK
#

objects = awk.obj

CFLAGS = /pointer_size=long /include="../../.."

awk.exe : $(objects)
	link $(objects),[-.-.awk]aseawk/library

awk.obj depends_on awk.c
