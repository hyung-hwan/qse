#
# OpenVMS MMS/MMK
#

objects = lsp.obj

CFLAGS = /include="../../.."
#CFLAGS = /pointer_size=long /include="../../.."

aselsp.exe : $(objects)
	link /executable=aselsp.exe $(objects),[-.-.lsp]aselsp/library

lsp.obj depends_on lsp.c
