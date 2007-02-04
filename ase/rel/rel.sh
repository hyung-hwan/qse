#!/bin/sh

finalize ()
{
	base="$1"; cur="$2"; dir="$3";

	if [ "$dir" = "" ]
	then
		cd "$base"
	else
		cd "$base/$dir"
	fi

	for i in *
	do
		if [ "$i" = "*" ]; then continue; fi
		if [ "$i" = "CVS" ]; then continue; fi
		if [ "$i" = "stx" ]; then continue; fi

		if [ "$cur" = "" ] 
		then
			file="$i"
			full="$base/$i"
		else
			file="$cur/$i"
			full="$base/$cur/$i"
		fi

		if [ -d "$full" ]
		then
			if [ "$dir" = "" ]
			then
				new="$i"
			else
				new="$dir/$i"
			fi

			finalize "$base" "$file" "$new"
			base="$1"; cur="$2"; dir="$3";
		elif [ -f "$full" ]
		then
			target="$base/ase"
			mkdir -p "$target/$cur"

			case "$full" in
			*.h|*.c|*.cc|*.cpp|*.java|*.awk|*.in)
				"$HOME/awk" -f "$base/rel/lic.awk" -a "$target/$file" "$full"
				;;
			*)
				cp -f "$full" "$target/$file"
				;;
			esac
			#echo "$full,$base,$file: $base/xxx/$file [OK]" 
		fi
	done
}


#if [ $# -ne 2 ]
#then
#	echo "Usage: $0"
#	exit 1
#fi

if [ ! -f ../CVS/Tag ]
then
	echo "Error: ../CVS/Tag not found"
	exit 1;
fi

VER=`cat ../CVS/Tag | cut -c2-`
echo $VER

cwd=`pwd`
cd ".."
base=`pwd`
finalize "$base" "" ""
exit 0
