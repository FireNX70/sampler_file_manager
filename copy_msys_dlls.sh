#!/bin/bash

#Based on https://janw.name/posts/3-msys2-dlls/

#paths=`find -L $1 -print0 -executable -type f | xargs -0 -I {} bash -c "echo \"{}\" | awk '{gsub(/ /, \"\\ \"); print;}'"`
os=`uname -o`

if [ ! "$os" == "Msys" ]
then
	echo "Use only in msys"
	exit 0
fi

bins=`find -L $1 -print -executable -type f`

#This fails on files with whitespaces in the filename because the for treats
#whitespaces as a delimiter, even if you escape them. The only way to change
#this is by setting the global IFS variable. None of the paths we care about
#have whitespaces, newlines, etc. so we're just gonna let it fail. Also, we're
#gonna be needlessly copying the same files to the same locations multiple
#times if there's multiple binaries in one directory, which is terrible both for
#efficiency and for NAND; but I'm tired of butting heads with this dumb fucking
#crap, so fuck it.
for path in $bins
do
	type=`file -b $path`

	if [[ $type =~ PE32.*executable.* ]]
	then
		ldd $path | grep /mingw64 | awk '{print $3}' | xargs -I {} cp {} `dirname $path`
	fi
done

windeployqt $(find -name sampler_file_manager.exe)
