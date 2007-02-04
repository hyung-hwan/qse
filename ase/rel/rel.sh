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
		if [ "$i" = "web.out" ]; then continue; fi

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
				"$AWK" -f "$BASE/rel/lic.awk" -a "$target/$file" "$full"
				;;
			*.man)
				html=`echo $i | sed 's/.man$/.html/'`
				"$AWK" -f "$BASE/rel/doc.awk" "$full" > "$SOURCE_ROOT/html/$html"
				"$AWK" -f "$BASE/rel/doc.awk" "$full" > "$DEPLOY_ROOT/$html"
				cp -f "$full" "$target/$file"
				;;
			*.css)
				cp -f "$full" "$target/$file"
				cp -f "$full" "$SOURCE_ROOT/html/$i"
				cp -f "$full" "$DEPLOY_ROOT/$i"
				;;
			*)
				cp -f "$full" "$target/$file"
				;;
			esac
		fi
	done
}

if [ ! -f ../CVS/Tag ]
then
	echo "Error: ../CVS/Tag not found"
	exit 1;
fi

AWK="$HOME/awk"

VERSION=`cat ../CVS/Tag | cut -c6- | tr '[A-Z]' '[a-z]' | sed 's/_/./g`

cwd=`pwd`
cd ".."
BASE=`pwd`
DEPLOY_ROOT="$BASE/web.out"
SOURCE_ROOT="$DEPLOY_ROOT/ase-$VERSION"

rm -rf "$DEPLOY_ROOT"
mkdir -p "$DEPLOY_ROOT"
mkdir -p "$SOURCE_ROOT"
mkdir -p "$SOURCE_ROOT/html"

finalize "" ""

cd "$DEPLOY_ROOT"
tar -cvf "ase-$VERSION.tar" "ase-$VERSION"
gzip "ase-$VERSION.tar"
rm -rf "ase-$VERSION"

exit 0
