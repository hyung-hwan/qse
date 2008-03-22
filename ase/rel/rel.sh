#!/bin/sh

finalize ()
{
	cur="$1"; dir="$2";

	if [ "$dir" = "" ]
	then
		cd "$BASE"
	else
		cd "$BASE/$dir"
	fi

	for i in *
	do
		if [ "$i" = "*" ]; then continue; fi
		if [ "$i" = "CVS" ]; then continue; fi
		if [ "$i" = ".svn" ]; then continue; fi
		if [ "$i" = "stx" ]; then continue; fi

		if [ "$cur" = "" ] 
		then
			file="$i"
			full="$BASE/$i"
		else
			file="$cur/$i"
			full="$BASE/$cur/$i"
		fi

		if [ -d "$full" ]
		then
			if [ "$dir" = "" ]
			then
				new="$i"
			else
				new="$dir/$i"
			fi

			finalize "$file" "$new"
			cur="$1"; dir="$2";
		elif [ -f "$full" ]
		then
			target="$SOURCE_ROOT/ase"
			mkdir -p "$target/$cur"

			case "$i" in
			*.h|*.c|*.cc|*.cpp|*.java|*.awk|*.in)
				"$ASEAWK" -f "$BASE/rel/lic.awk" -a "$target/$file" "$full"
				;;
			*.man)
				html=`echo $i | sed 's/.man$/.html/'`
				"$ASEAWK" -explicit -noimplicit -f "$BASE/rel/doc.awk" "$full" > "$SOURCE_ROOT/html/$html"
				"$ASEAWK" -explicit -noimplicit -f "$BASE/rel/doc.awk" "$full" > "$ASETGT/$html"
				cp -f "$full" "$target/$file"
				;;
			*.css)
				cp -f "$full" "$target/$file"
				cp -f "$full" "$SOURCE_ROOT/html/$i"
				cp -f "$full" "$ASETGT/$i"
				;;
			*.dsp|*.dsw|*.sln|*.vcproj|*.csproj|*.bat|*.cmd)
				"$ASEAWK" -f "$BASE/rel/unix2dos.awk" "$full" > "$target/$file"	
				;;
			descrip.mms)
				"$ASEAWK" -f "$BASE/rel/unix2dos.awk" "$full" > "$target/$file"	
				;;
			*.frx)
				cp -f "$full" "$target/$file"
				;;
			*)
				if [ "$dir" = "test/com" ]
				then
					"$ASEAWK" -f "$BASE/rel/unix2dos.awk" "$full" > "$target/$file"	
				else
					cp -f "$full" "$target/$file"
				fi
				;;
			esac
		fi
	done
}


print_usage ()
{
	echo "Usage: $0 awk version target archiver"
	echo "where awk := full path to aseawk"
	echo "      version := any string"            
	echo "      target := full path to the target directory"
	echo "      archiver := gzip | zip"
}

############################
# BEGINNING OF THE PROGRAM #
############################

if [ $# -ne 4 ]
then
	print_usage "$0"
	exit 1
fi

if [ "$4" != "gzip" -a "$4" != "zip" ]
then
	print_usage "$0"
	exit 1
fi

ASEAWK="$1"
ASEVER="$2"
ASETGT="$3"
ASEARC="$4"

CURDIR=`pwd`
cd ".."
BASE=`pwd`

SOURCE_ROOT="$ASETGT/ase-$ASEVER"

rm -rf "$ASETGT"
mkdir -p "$ASETGT"
mkdir -p "$SOURCE_ROOT"
mkdir -p "$SOURCE_ROOT/html"

finalize "" ""

cd "$ASETGT"

if [ "$ASEARC" = "gzip" ]
then
	tar -cvf "ase-$ASEVER.tar" "ase-$ASEVER"
	gzip "ase-$ASEVER.tar"
	#mv "ase-$ASEVER.tar.gz" "ase-$ASEVER.tgz"
elif [ "$ASEARC" = "zip" ]
then
	ls -l
	echo zip -r "ase-$ASEVER" "ase-$ASEVER"
	zip -r ase "ase-$ASEVER"
	mv -f ase.zip "ase-$ASEVER.zip"
fi

rm -rf "ase-$ASEVER"

cd "$CURDIR"
exit 0
