#!/bin/bash
# $Id: uade-gLauncher.sh,v 1.2 2004/08/10 15:52:32 mld Exp $

## vars
DIALOG=${DIALOG=zenity}
tempfile=`tempfile 2>/dev/null` || tempfile=/tmp/uadeklauncher$$
echo "none" > $tempfile

if [ "`which uade 2>/dev/null`" ]; then
UADEPREFIX=""
else
UADEPREFIX="$HOME/.uade/"
#UADEPREFIX="$HOME/.uade/bin/"
#UADEPREFIX="$HOME/.xmms/uade/"
#UADEPREFIX="<edit path...>"
fi

UADEEXE="$UADEPREFIX""uade -int -repeat -force"
UADECMD="$UADEEXE"
UADEINFO="no info available"
uadecommand=""
uadepid="no"
uadestatus="stopped"

filename="<none>"
filepwd=""
subsong="0"

# -------------------------------------------------------------------------
function killuade {
# parameters:
# none
# return:
# none
# -------------------------------------------------------------------------
   if [ "$uadepid" != "no" ];then  
     kill $uadepid
        uadepid="no"
	uadestatus="stopped"
   fi
}

# -------------------------------------------------------------------------
function openfile {
# parameters:
# $1 filename
# return:
# $retval
# -------------------------------------------------------------------------
if [ "$1" = "" ];then
   $DIALOG --title "UADE gLauncher (Open File...)" \
   --file-selection --filename "$filepwd" > "$tempfile" 1>&1
   retval=$?
else
 echo "$@" >"$tempfile"
 retval=0
fi

case $retval in
  0)
  filepath=`cat "$tempfile"`
  filename=`cat "$tempfile" | sed 's/.*\(\/\\)//'` #strip path
  filepwd=`cat "$tempfile" |sed 's/[^/]*$//'` #strip filename
  UADECMD="$UADEEXE"
  playfile
  # get mod infos 
   `$HOME/.uade/bin/uade 2>$tempfile "$filepath" -t 3 -outpipe 1 >/dev/null`
   modname=`cat $tempfile |grep "modulename"| sed 's/.*modulename: //'`
   playername=`cat $tempfile|grep "playername"| sed 's/.*playername: //'`
   formatname=`cat $tempfile |grep "formatname"| sed 's/.*formatname: //'`
   cursubsong=`cat $tempfile |grep "subsong"| sed 's/.*current: //'`
   maxsubsong=`cat $tempfile |grep "subsong"| sed 's/.*maximum: //'|sed 's/ .*//'`
   minsubsong=`cat $tempfile |grep "subsong"| sed 's/.*minimum: //'|sed 's/ .*//'`
   subsong=$cursubsong
  ;;
  1)
    echo "Cancel pressed.";;
  255)
    if test -s $tempfile ; then
      cat $tempfile
    else
      echo "ESC pressed."
    fi
    ;;
esac
}

# -------------------------------------------------------------------------
function playfile {
# parameters:
# none
# return:
# none
# -------------------------------------------------------------------------
    killuade
    $UADECMD "$filepath" >/dev/null 2>&1&
    uadepid=`echo $!`
    uadestatus="playing..."
}
# -------------------------------------------------------------------------
function exituade {
# parameters:
# none
# return:
# none
# -------------------------------------------------------------------------
    killuade
    trap "rm -f $tempfile" 0 1 2 5 15
    exit 0
}


#### main 

## filename given, or request file
openfile "$1"
while :
do
    $DIALOG --title "UADE gLauncher ($cat $filename)" \
          --list --width=555 --column "--=[ UADE - Unix Amiga Delitracker Emulator ]=--
\
        File: $filename
	Status: $uadestatus

	Module: $modname
	Moduleformat: $playername  $formatname 
	Subsong: $subsong [min: $minsubsong, max: $maxsubsong]"\
	"Play" \
	"Stop" \
	"Next Subsong"\
	"Previous Subsong"\
	"Open new song"\
	"Exit" >$tempfile 1>&1
      retval=$?
      uadecommand="$(cat $tempfile)"


 case $retval in
  0)

## stop file
  if [ "$uadecommand" = "Stop" ];then
   killuade
  fi


## play file
  if [ "$uadecommand" = "Play" ];then
        playfile
  fi

   if [ "$uadecommand" = "Next Subsong" ];then
    subsong=`echo $[$subsong+1]`
    UADECMD="$UADEEXE -sub $subsong"
    playfile
   fi

    if [ "$uadecommand" = "Previous Subsong" ];then
      if [ "$subsong" != "0" ];then
      subsong=`echo $[$subsong-1]`
           if [ "$subsong" != "0" ];then
            UADECMD="$UADEEXE -sub $subsong"
           else
            UADECMD="$UADEEXE"
	   fi
      playfile
     fi
    fi
   
   if [ "$uadecommand" = "Open new song" ];then
    openfile ""
   fi

   if [ "$uadecommand" = "Exit" ];then
    exituade
   fi
   ;;
  1)
    exituade
   ;;
  255)
      exituade
   ;;
 esac
done