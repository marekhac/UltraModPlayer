#!/bin/sh

# script for unpacking example modules from wanted team lha/lzx archives

function error {
    echo "FATAL ERROR:" "$@"
    exit -1
}

for i in "$@" ; do
    if test ! -f "$i" ; then
	echo $i is not a regular file. ignoring.
	continue
    fi
    base="`echo $i |sed -e "s|\(.*\)\.\(.*\)|\1|"`"
    ext="`echo $i |sed -e "s|\(.*\)\.\(.*\)|\2|"`"
    # echo base: $base extension: $ext

    lead="`echo $base |sed -e "s|\(.*\)_\(.*\)|\2|"`"
    if test "$lead" != "$base" ; then
	echo base $base lead $lead
	base="$lead"
    fi

    if test ! -d "$base" ; then
	mkdir "$base" || error "can not create dir $base"
    fi
    cd "$base" || error "could not cd to $base"
    if test "$ext" = "lha" ; then
	lha x "../$i"
    else
	unlzx -x "../$i"
    fi
    cd ..
done
