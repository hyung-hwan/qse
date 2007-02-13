#
# OpenVMS MMS/MMK
#

objects = awk.obj

CFLAGS = /include="../../.."
#CFLAGS = /pointer_size=long /include="../../.."

aseawk.exe : $(objects)
	link /executable=aseawk.exe $(objects),[-.-.awk]aseawk/library

awk.obj depends_on awk.c
