#
# Taken from message #8926 by nburns1980 in the sed-users mailing list
#
\|cp -a /bin/\* /ramdisk/busybin/|,+1 {
//n
\|echo -n "done\${CRE}"| a\
#COPY DISK TO /ramdisk/cdrom\
if [ -n "$FROMINIT" ];then\
	cp -af /image/* /ramdisk/cdrom/\
fi
}
