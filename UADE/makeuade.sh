#!/bin/sh

if test -z $1; then
 echo "you must give a destination path!"
 exit
fi
if test -d $1; then
 echo "directory $1 already exists!"
 exit
fi

mkdir -p $1

# copy directories
cp -r amigasrc contrib decrunch dist docs effects frontends osdep players plugindir src uade-docs $1/

# copy main dir files
cp README INSTALL* COPYING BUGS FIXED TESTING *PLANS ChangeLog.txt $1/
cp configure configure.h Makefile.in xmms-error.txt $1/
cp score uaerc $1/
cp makeuade.sh $1/

# copy some music
mkdir $1/songs
mkdir $1/songs/ahx
cp -f songs/ahx/AHX.Cruisin* $1/songs/ahx/
mkdir $1/songs/future_composer
cp -f songs/future_composer/fc13.smod7* $1/songs/future_composer/
mkdir $1/songs/custom
cp -f songs/custom/cust.VisionFactory2* $1/songs/custom/

echo "now you may want to execute:"
echo "find $1/ -type d |grep CVS |xargs rm -rf"
