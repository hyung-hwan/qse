#
# Taken from message #8918 by Davide Brini in the sed-users mailing list
#

$!N

\|^cp -a /bin/\* /ramdisk/busybin/\necho -n "done\${CRE}"$| {
	s|$|\
#COPY DISK TO /ramdisk/cdrom\
if [ -n "$FROMINIT" ];then\
	cp -af /image/* /ramdisk/cdrom/\
fi|
	p
	d
}

P
D
