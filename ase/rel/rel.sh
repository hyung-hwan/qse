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
			root="$BASE/ase-$VER"
			target="$root/ase"
			mkdir -p "$target/$cur"

			case "$full" in
			*.h|*.c|*.cc|*.cpp|*.java|*.awk|*.in)
				"$HOME/awk" -f "$BASE/rel/lic.awk" -a "$target/$file" "$full"
				;;
			*)
				cp -f "$full" "$target/$file"
				;;
			esac
			#echo "$full,$BASE,$file: $BASE/xxx/$file [OK]" 
		fi
	done
}

if [ ! -f ../CVS/Tag ]
then
	echo "Error: ../CVS/Tag not found"
	#exit 1;
fi

VER=`cat ../CVS/Tag | cut -c6- | tr '[A-Z]' '[a-z]' | sed 's/_/./g`
VER="0.1.0"

cwd=`pwd`
cd ".."
BASE=`pwd`

finalize "" ""

exit 0
