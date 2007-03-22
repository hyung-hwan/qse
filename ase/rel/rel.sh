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
				"$ASEAWK" -f "$BASE/rel/doc.awk" "$full" > "$SOURCE_ROOT/html/$html"
				"$ASEAWK" -f "$BASE/rel/doc.awk" "$full" > "$ASETGT/$html"
				cp -f "$full" "$target/$file"
				;;
			*.css)
				cp -f "$full" "$target/$file"
				cp -f "$full" "$SOURCE_ROOT/html/$i"
				cp -f "$full" "$ASETGT/$i"
				;;
			*.dsp|*.dsw)
				"$ASEAWK" -f "$BASE/rel/unix2dos.awk" "$full" > "$target/$file"	
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


############################
# BEGINNING OF THE PROGRAM #
############################

if [ $# -ne 3 ]
then
	echo "Usage: $0 awk version target"
	echo "where awk := full path to aseawk"
	echo "      version := any string"            
	echo "      target := full path to the target directory"
	exit 1
fi

ASEAWK="$1"
ASEVER="$2"
ASETGT="$3"

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
tar -cvf "ase-$ASEVER.tar" "ase-$ASEVER"
gzip "ase-$ASEVER.tar"
mv "ase-$ASEVER.tar.gz" "ase-$ASEVER.tgz"
rm -rf "ase-$ASEVER"

cd "$CURDIR"
exit 0
