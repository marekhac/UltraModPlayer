#!/bin/sh
# $Id: uade-kLauncher.sh,v 1.8 2004/09/15 06:50:30 mld Exp $
# Lincense: GPL
##

###-------------------------------------------------------------------------###
### GLOBAL
MSG_VERSION="0.10"
MSG_NAME="molusc.sh"
MSG_FULLNAME="UADE::Mod'n'Others LaUncher SCript"

INP_global_opt_sub=0
INP_global_opt_ntsc=""
INP_global_opt_pan=""

TEMPFILE=`tempfile 2>/dev/null` || TEMPFILE=/tmp/$MSG_NAME$$
echo >$TEMPFILE
trap "rm -f $TEMPFILE" 0 1 2 5 15

###-------------------------------------------------------------------------###
### Input
function INP_available {
INP_list=""
 for iline in $(echo $INP_list_sys); do
  INP_$iline available
  if [ "$rts" = "yes" ]; then
   INP_list="$INP_list $iline"
  fi
 done
  #echo DEBUG: Input: players $INP_list found.
}

function INP_is_our_file {
 for iline in $(echo $INP_list); do
  INP_$iline is_our_file "$SONGFILE"
   #echo DEBUG: Input: player $iline returned $rts for $SONGFILE
  if [ "$rts" = "yes" ]; then
   PLAYER="$iline"
   INP_global_opt_sub=$MINSUBSONG
   return
  fi
  done
}

function INP_play {
   echo DEBUG: player $PLAYER plays $1   
   if [ "$PLAYER" = "" ];then
   echo "$MSG_NAME: no suitable player for this file found"
   else
   INP_stop
   STATUS="played with $PLAYER player"
   INP_$PLAYER play "$1"
   PLAYERPID=$rts
   fi
}

function INP_stop {
   echo DEBUG: player $PLAYERPID stopped
   if [ "$PLAYERPID" != "" ];then  
     kill $PLAYERPID >/dev/null
     PLAYERPID=""
     STATUS="stopped"
   fi
}

function INP_set_option {

   if [ "$PLAYER" = "" ];then
    echo "$MSG_NAME: no suitable player for this file found"
   else
   #echo DEBUG: player $PLAYER sets option $1
   INP_$PLAYER set_option "$1"
   fi
}

function INP_list_options {
   #echo DEBUG: player $PLAYER lists options
   INP_$PLAYER list_options
}

function INP_get_info {
   #echo DEBUG: player $PLAYER lists options
   INP_$PLAYER get_info
}

CONV_list_sys="m3u"
INP_list_sys="$CONV_list_sys mpg321 ogg123 uade adplay xmp null"

#INP_list_sys="$CONV_list_sys mpg321 ogg123 uade xmp sidplay2 null"
###-------------------------------------------------------------------------###
### M3U Handling (handle converters and m3u as input module)
### we always return false for songs, to pass our converted data to the rest of the ### input modules

function INP_m3u {
  rts=""  
  if [ "$1" = "available" ]; then
   rts=yes	## m3u handling always available
  return
  
  elif [ "$1" = "is_our_file" ];then
   if test -z `cat "$SONGFILE" | grep '#EXTM3U'` ; then
    SONGFILE="$2"
    return
   else
      if test -z `echo "$SONGFILE" | sed -n 's/.*\.\(.*\)/\1/p'|grep 'm3u'` ; then
       MISC_init_plst
       MISC_plst=`printf "%s\n#EXTINF:%s\n%s\n" "$MISC_plst" "<-- [ Back ]" "$OLDM3UFILE"`
       MISC_add_plst_to_plst "$SONGFILE"
       TRANS_load_file $SONGFILE # load the first file from playlist
       rts=""
      else
       OLDM3UFILE="$2"
       MISC_define_as_plst "$2"
       rts="yes" ## first song is another playlist, don't play.
      fi
    return
   fi
  elif [ "$1" = "play" ];then
   STATUS="Playlist loaded"
   rts=""
   return
  
  elif [ "$1" = "set_option" ];then
   :
   return

  elif [ "$1" = "list_options" ];then
   :
   return
  
  elif [ "$1" = "get_info" ];then
   GUI_songinfo="M3U Playlist"
   return
  fi
}
function INP_null {
  rts=""  
  if [ "$1" = "available" ]; then
   rts=yes	## m3u handling always available
  return
  
  elif [ "$1" = "is_our_file" ];then
   rts=yes
  elif [ "$1" = "play" ];then
   STATUS="idle"
   return
  
  elif [ "$1" = "set_option" ];then
   :
   return

  elif [ "$1" = "list_options" ];then
   :
   return
  
  elif [ "$1" = "get_info" ];then
   GUI_songinfo="<no song>"
   return
  fi
}

###-------------------------------------------------------------------------###
### UADE

function INP_uade {
  rts=""  
  if [ "$1" = "available" ]; then
   	 rts="yes"
   	 if [ "`which uade 2>/dev/null`" ]; then
  	 	  INP_uade_prefix=""
  	 	 else
		 if test -x "$HOME/.uade/uade"; then
  	 	   INP_uade_prefix="$HOME/.uade/"
  	 	 else
  	 	    rts=""
  	 	    #INP_uade_prefix="$HOME/.uade/bin/"
  	 	    #INP_uade_prefix="$HOME/.xmms/uade/"
  	 	    #INP_uade_prefix="<edit path...>"
  	 	  fi
   	 fi  
  INP_uade_defaults="-repeat -force"
  INP_uade_exe="$INP_uade_prefix""uade"
  return
  
  elif [ "$1" = "is_our_file" ];then
  	`"$INP_uade_exe" 2>$TEMPFILE "$2" -t 3 -one -outpipe 1 >/dev/null`
	 if [ "`cat $TEMPFILE|grep 'unknown format'`" = "" ]; then
  		  ## set default values 
  		  INP_global_opt_sub=0
  		  INP_global_opt_ntsc=""
		  
		  SONGINFO=`cat $TEMPFILE`
		  SONGNAME=`echo -e "$SONGINFO" |grep "modulename"| sed 's/.*modulename: //'`
		  if [ "$SONGNAME" = "" ] ; then
		   SONGNAME=`basename "$2"`
		  fi  
		  PLAYERNAME=`echo -e "$SONGINFO" |grep "playername"| sed 's/.*playername: //'`
    		  FORMATNAME=`echo -e "$SONGINFO" |grep "formatname"| sed 's/.*formatname: //'`
		  CURSUBSONG=`echo -e "$SONGINFO" |grep "subsong"| sed 's/.*current: //'`
		  MAXSUBSONG=`echo -e "$SONGINFO" |grep "subsong"| sed 's/.*maximum: //'|sed 's/ .*//'`
		  MINSUBSONG=`echo -e "$SONGINFO" |grep "subsong"| sed 's/.*minimum: //'|sed 's/ .*//'` 
		  
		  #SONGFILE="$2"
  		  rts="yes"
   fi
  return
  
  elif [ "$1" = "play" ];then
  	    INP_uade_cmd="$INP_uade_exe $INP_uade_defaults $INP_uade_options"
  	    $INP_uade_cmd "$2" >/dev/null 2>&1&
  	    rts="$!"
  return
 
  elif [ "$1" = "set_option" ];then
  	 if [ "$2" = "pan" ];then
  		  if [ "$INP_global_opt_pan" = "on" ]; then
  		     INP_global_opt_pan=""
  		    else
  		     INP_global_opt_pan="on"
  		   fi
      	elif [ "$2" = "ntsc" ];then
  	   if [ "$INP_global_opt_ntsc" = "on" ]; then
  	     INP_global_opt_ntsc=""
  	    else
  	     INP_global_opt_ntsc="on"
  	    fi
   	elif [ "$2" = "sub_next" ];then
  	  INP_global_opt_sub=$[$INP_global_opt_sub+1]
   
   	elif [ "$2" = "sub_prev" ];then
         if [ "$INP_global_opt_sub" != "0" ];then
  	  INP_global_opt_sub=$[$INP_global_opt_sub-1]
	 fi    
   fi
  
  # create player specific options
  INP_uade_options="${INP_global_opt_pan:+-pan 0.8} ${INP_global_opt_ntsc:+-ntsc} ${INP_global_opt_sub:+-sub $INP_global_opt_sub}"
   return
  
  elif [ "$1" = "list_options" ];then
    ### what options does our player support ?
    rts="sub ntsc pan"
  return
  elif [ "$1" = "get_info" ];then
   ## each Plugin sets its own GUI_songinfo text
     GUI_songinfo=`printf "Song: %s\nFormat: %s %s\nSubsong: %s [min: %s, max: %s]\n" "$SONGNAME" "$PLAYERNAME" "$FORMATNAME" $CURSUBSONG $MINSUBSONG $MAXSUBSONG` 
  return
  
  fi
}

###-------------------------------------------------------------------------###
### XMP 
function INP_xmp {
  rts=""

  if [ "$1" = "available" ]; then
  	  rts="yes"
  	  if [ "`which xmp 2>/dev/null`" ]; then
  	   xmp_uade_prefix=""
  	  else
  	   rts=""
  	   #INP_xmp_prefix="$HOME/bin/"
  	  fi
  	INP_xmp_defaults=" --loop"
  	INP_xmp_exe="$INP_xmp_prefix"xmp
  	return

  elif [ "$1" = "is_our_file" ];then
    echo "`$INP_xmp_exe 2>&1 --load-only "$2"`" >$TEMPFILE
    	  SONGINFO=`cat $TEMPFILE`
    if [ "`echo -e "$SONGINFO"|grep 'Estimated time'`" != "" ]; then
	  INP_xmp_options=""
	  
	  SONGNAME=`echo -e "$SONGINFO" |grep "Module title"| sed 's/.*Module title   : //'`
	  PLAYERNAME=`echo -e "$SONGINFO" |grep "Tracker name"| sed 's/.*Tracker name   : //'`
	  FORMATNAME="[`echo "$SONGINFO" |grep "Module type"| sed 's/.*Module type    : //'`]"
	  #SONGFILE="$2"
	  rts="yes"
    fi
  return
  
  elif [ "$1" = "play" ];then
     INP_xmp_cmd="$INP_xmp_exe $INP_xmp_defaults $INP_xmp_options"
     $INP_xmp_cmd -q $INP_xmp_defaults "$2" >/dev/null 2>&1&
     rts="$!"
  return
 
  elif [ "$1" = "get_info" ];then
       GUI_songinfo=`printf "Song: %s\nFormat: %s %s\n" "$SONGNAME" "$PLAYERNAME" "$FORMATNAME"` 
  return
  fi
    if [ "$1" = "set_option" ];then
   if [ "$2" = "pan" ];then
     if [ "$INP_global_opt_pan" = "on" ]; then
       INP_global_opt_pan=""
      else
       INP_global_opt_pan="on"
     fi
    fi
  # create player specific options
   INP_xmp_options="${INP_global_opt_pan:---pan 0}"
   return
  
  elif [ "$1" = "list_options" ];then
  rts="pan"
  return
  fi
}

###-------------------------------------------------------------------------###
### OGG Vorbis
function INP_ogg123 {
  rts=""

  if [ "$1" = "available" ]; then
    rts="yes"
    if [ "`which ogg123 2>/dev/null`" ]; then
     INP_ogg_prefix=""
    else
     rts=""
     #INP_ogg_prefix="$HOME/bin/"
    fi
  INP_ogg_exe="$INP_ogg_prefix"ogg123
  return

  elif [ "$1" = "is_our_file" ];then
     if [ "`file "$2"|grep 'Ogg data, Vorbis audio'`" != "" ]; then
         INP_ogg_options=""
	 SONGFILE="$2"
         rts="yes"
     else
      $INP_ogg_exe "$2" -K 1 -d null 2>$TEMPFILE
      if [ "`cat $TEMPFILE|grep 'Ogg Vorbis stream'`" != "" ]; then
       INP_ogg_options=""
       rts="yes"
      fi
    fi
  return
  
  elif [ "$1" = "play" ];then
     INP_ogg_cmd="$INP_ogg_exe $INP_ogg_defaults $INP_ogg_options"
     $INP_ogg_cmd -q "$2" &
     rts="echo $!"
  return

  elif [ "$1" = "get_info" ];then
  GUI_songinfo=`printf "Song: %s\nFormat: %s\n" "$(basename "$SONGFILE")" "Ogg Vorbis"`
  return
  
  elif [ "$1" = "list_options" ];then
  rts=""
  return
  fi
}
###-------------------------------------------------------------------------###
### MP3
function INP_mpg321 {
  rts=""

  if [ "$1" = "available" ]; then
    rts="yes"
    if [ "`which mpg321 2>/dev/null`" ]; then
     INP_mp3_prefix=""
    else
     rts=""
     #INP_mp3_prefix="$HOME/bin/"
    fi
  INP_mp3_exe="$INP_mp3_prefix"mpg321
  return

  elif [ "$1" = "is_our_file" ];then
     if [ "`file "$2"|grep 'MP3'`" != "" ]; then
         INP_mp3_options=""
	 SONGFILE="$2"
         rts="yes"
    fi
  return
  
  elif [ "$1" = "play" ];then
     INP_mp3_cmd="$INP_mp3_exe $INP_mp3_defaults $INP_mp3_options"
     $INP_mp3_cmd -q "$2" &
     rts="$!"
  return

  elif [ "$1" = "get_info" ];then
  GUI_songinfo=`printf "Song: %s\nFormat: %s\n" "$(basename "$SONGFILE")" "MP3"`
  return
  
  elif [ "$1" = "list_options" ];then
  rts=""
  return
  fi
}
###-------------------------------------------------------------------------###
### adplay
function INP_adplay {
  rts=""

  if [ "$1" = "available" ]; then
    rts="yes"
    if [ "`which adplay 2>/dev/null`" ]; then
     INP_adplay_prefix=""
    else
     rts=""
     #INP_adplay_prefix="$HOME/bin/"
    fi
  INP_adplay_exe="$INP_adplay_prefix"adplay
  INP_adplay_defaults=""
  return

  elif [ "$1" = "is_our_file" ];then
   echo "">$TEMPFILE
   $INP_adplay_exe 2>$TEMPFILE -r -O disk -d /dev/null "$2" &
   rts="`echo $!`"
   sleep 1
   kill $rts
   SONGINFO=`cat $TEMPFILE`
     if [ "`echo -e "$SONGINFO"|grep 'Subsong'`" != "" ]; then
         INP_adplay_options=""
	 	  SONGINFO=`cat $TEMPFILE`
		  SONGNAME="$(echo -e "$SONGINFO" |grep "Author"| sed 's/.*Author: //')-$(echo -e "$SONGINFO" |grep 'Title'| sed 's/.*Title : //')"
		  if [ "$SONGNAME" = "-" ] ; then
		   SONGNAME=`basename "$2"`
		  fi  
    		  PLAYERNAME=`echo -e "$SONGINFO" |grep "Type"| sed 's/.*Type  : //'`
		  CURSUBSONG=`echo -e "$SONGINFO" |grep "Subsong"`
		  MAXSUBSONG=`echo -e "$CURSUBSONG" | sed 's/,.*//'|sed 's/.*\///'
 `
		  MINSUBSONG=`echo -e "$CURSUBSONG" | sed 's/.*Subsong: //'|sed 's/\/.*//'`
		  CURSUBSONG=$MINSUBSONG
	 SONGFILE="$2"
         rts="yes"
    fi
  return
  
  elif [ "$1" = "play" ];then
     INP_adplay_cmd="$INP_adplay_exe $INP_adplay_defaults $INP_adplay_options"
     $INP_adplay_cmd 2>/dev/null "$2" &
     rts="$!"
  return

  elif [ "$1" = "get_info" ];then
  GUI_songinfo=`printf "Song: %s\nFormat: %s\nSubsong: %s [min: %s, max: %s]\n" "$SONGNAME" "$PLAYERNAME" $CURSUBSONG $MINSUBSONG $MAXSUBSONG`
  return
  
  elif [ "$1" = "list_options" ];then
  rts="sub pan"
  return
   elif [ "$1" = "set_option" ];then
  	 if [ "$2" = "pan" ];then
  		  if [ "$INP_global_opt_pan" = "on" ]; then
  		     INP_global_opt_pan=""
  		    else
  		     INP_global_opt_pan="on"
  		   fi

   	elif [ "$2" = "sub_next" ];then
  	  INP_global_opt_sub=$[$INP_global_opt_sub+1]
   
   	elif [ "$2" = "sub_prev" ];then
         if [ "$INP_global_opt_sub" != "0" ];then
  	  INP_global_opt_sub=$[$INP_global_opt_sub-1]
	 fi
	fi    
	INP_adplay_options="${INP_global_opt_pan:+--stereo} ${INP_global_opt_sub:+-s $INP_global_opt_sub}"
	return  
  fi
}

###-------------------------------------------------------------------------###
### Sidplay2
function INP_sidplay2 {
  rts=""

  if [ "$1" = "available" ]; then
    rts="yes"
    if [ "`which sidplay2 2>/dev/null`" ]; then
     INP_sid_prefix=""
    else
     rts=""
     #INP_sid_prefix="$HOME/bin/"
    fi
  INP_sid_exe="$INP_sid_prefix"sidplay2
  INP_sid_defaults=" -ol"
  return

  elif [ "$1" = "is_our_file" ];then
     if [ "`file "$2"|grep 'sidtune'`" != "" ]; then
         INP_sid_options=""
	 SONGFILE="$2"
         rts="yes"
    fi
  return
  
  elif [ "$1" = "play" ];then
     INP_sid_cmd="$INP_sid_exe $INP_sid_defaults $INP_sid_options"
     $INP_sid_cmd -q "$2" &
     rts="$!" ## return PID of adplay
  return

  elif [ "$1" = "get_info" ];then
  GUI_songinfo=`printf "Song: %s\nFormat: %s\n" "$(basename "$SONGFILE")" "C64 SID tune"`
  return
  
  elif [ "$1" = "list_options" ];then
  rts="sub ntsc pan"
  return
  
    elif [ "$1" = "set_option" ];then
  	 if [ "$2" = "pan" ];then
  		  if [ "$INP_global_opt_pan" = "on" ]; then
  		     INP_global_opt_pan=""
  		    else
  		     INP_global_opt_pan="on"
  		   fi
      	elif [ "$2" = "ntsc" ];then
  	   if [ "$INP_global_opt_ntsc" = "on" ]; then
  	     INP_global_opt_ntsc=""
  	    else
  	     INP_global_opt_ntsc="on"
  	    fi
   	elif [ "$2" = "sub_next" ];then
  	  INP_global_opt_sub=$[$INP_global_opt_sub+1]
   
   	elif [ "$2" = "sub_prev" ];then
         if [ "$INP_global_opt_sub" != "0" ];then
  	  INP_global_opt_sub=$[$INP_global_opt_sub-1]
	 fi  
	fi
	 # create player specific options
  INP_sid_options="${INP_global_opt_pan:+-s} ${INP_global_opt_ntsc:+-vn} ${INP_global_opt_sub:+-o$INP_global_opt_sub}"
	echo $INP_sid_options
   	return
  fi
}

###-------------------------------------------------------------------------###
### TRANSPORT
###-------------------------------------------------------------------------###
function TRANS_available {
INP_list=""
 for tline in $(echo $TRANS_list_sys); do
  TRANS_$tline available
  if [ "$rts" = "yes" ]; then
   TRANS_list="$TRANS_list $tline"
  fi
 done
  #echo DEBUG: TRANS: transport message $TRANS_list found.
}

function TRANS_is_our_file {
 for tline in $(echo $TRANS_list); do
  TRANS_$tline is_our_file "$1"
    if [ "$rts" = "yes" ]; then
   LOADER="$tline"
   #echo DEBUG: TRANS: loader $LOADER will transport $SONGFILE
   SONGPWD=`dirname "$SONGFILE"`
   return
  fi
  done
}

function TRANS_load_file {
 TRANS_is_our_file "$1"
# for tline in $(echo $TRANS_list); do
    TRANS_$LOADER load_file "$1"
      if [ "$rts" != "yes" ] ; then
       SONGFILE=""
       fi
#  done
}

TRANS_list_sys="kfm local"

###-------------------------------------------------------------------------###
### local files
function TRANS_local {
  if [ "$1" = "available" ];then
  rts=yes 	## we can always load local file can we *g*
  return
  
  elif [ "$1" = "is_our_file" ];then
   rts="yes"
  return
  
  elif [ "$1" = "load_file" ];then
    if test -r "$2"; then
     rts="yes" ##return yes for successful load
     return
    else 
    rts=""	##return false for load error
     return
    fi
  fi
}

function TRANS_kfm {
  if [ "$1" = "available" ];then
     if [ "`which kfmclient 2>/dev/null`" ]; then
      MODSAVEPATH="$HOME/.uade/cachedmods"
      rts="yes"
     else
      rts=""
    fi
  return
  
  elif [ "$1" = "is_our_file" ];then
   if test -z `echo $SONGFILE|grep "tp://"` ; then
     #echo "using local file: $SONGFILE"
     rts=""
   else
     rts=yes
   fi
  return
  
  elif [ "$1" = "load_file" ];then
  SONGDIR=`dirname "$SONGFILE"|sed -e s/["http:"]*//|sed s/["//"]*//`
  SONGNAME=`basename "$SONGFILE"`
  
   if [ "`echo "$SONGFILE"| grep "http://"|sed -n 's/.*\.\(.*\)/\1/p'|grep 'm3u'`" = "m3u" ] ; then
       kfmexec cat "$SONGFILE">$TEMPFILE
      SONGFILE="$TEMPFILE"
  else
    mkdir -p "$MODSAVEPATH/$SONGDIR"
    if test -r "$MODSAVEPATH"/"$SONGDIR"/"$SONGNAME"; then
    echo "using cached local file: $MODSAVEPATH"/"$SONGDIR"/"$SONGNAME" 
   else
    kfmclient copy "$SONGFILE" "$MODSAVEPATH"/"$SONGDIR/"
   fi
   SONGFILE="$MODSAVEPATH"/"$SONGDIR"/"$SONGNAME"
  fi
   
   rts=yes	##return the path
  fi
}
###-------------------------------------------------------------------------###
### GUI
###-------------------------------------------------------------------------###
function GUI_available {
INP_list=""
 for line in $(echo $GUI_list_sys); do
  GUI_$line available
  if [ "$rts" = "yes" ]; then
   GUI_list="$GUI_list $line"
  fi
 done
  #echo DEBUG: GUI: user interfaces $GUI_list found.
}

function GUI_is_active {
 for line in $(echo $GUI_list); do
  GUI_$line is_active
  if [ "$rts" = "yes" ]; then
   GUI="$line"
   #echo DEBUG: user interface $GUI
   return
  fi
  done
}

function GUI_open_main {
   #echo DEBUG: User Interface $GUI opens main window
   INP_$PLAYER get_info
   GUI_$GUI open_main
   GUI_command=$rts
   #echo DEBUG: User Interface $GUI returned $GUI_command
}
function GUI_build_main {
   #echo DEBUG: User Interface $GUI building main window
   GUI_$GUI build_main
}

function GUI_open_loadfile {
   #echo DEBUG: User Interface $GUI opens Loadfile window
   GUI_$GUI open_loadfile
   if [ "$rts" != "" ]; then
    SONGFILE="$rts"
   #echo DEBUG: User Interface $SONGFILE chosen
   fi
}

function GUI_open_plst {
   #echo DEBUG: User Interface $GUI opens Playlist window
   GUI_$GUI open_plst
   if [ "$rts" != "" ]; then
    SONGFILE="$rts"
   fi
}
function GUI_build_plst {
   #echo DEBUG: User Interface $GUI builds Playlist
   GUI_$GUI build_plst
}

GUI_list_sys="kdialog"

###-------------------------------------------------------------------------###
### kdialog
function GUI_kdialog {
  if [ "$1" = "available" ];then
  rts=yes
  return
  
  elif [ "$1" = "is_active" ];then
  DIALOG=${DIALOG=kdialog}
  rts=yes
  return
  
  elif [ "$1" = "open_main" ];then 
      ITEMS=`printf "\"%s\" \"%s\"\n" "song_play" "Play"`
      ITEMS=`printf "%s \"%s\" \"%s\"\n" "$ITEMS" "song_stop" "Stop"`
      INP_$PLAYER list_options
      for bline in $(echo $rts); do
        if [ "$bline" = "ntsc" ] ; then
	    ITEMS=`printf "%s \"%s\" \"%s\"\n" "$ITEMS" "opt_ntsc" "Toggle NTSC speed(${INP_global_opt_ntsc:-off})"`
	elif [ "$bline" = "pan" ] ; then
	    ITEMS=`printf "%s \"%s\" \"%s\"\n" "$ITEMS" "opt_pan" "Toggle Panning (${INP_global_opt_pan:-off})"`
	elif [ "$bline" = "sub" ] ; then 
	    ITEMS=`printf "%s \"%s\" \"%s\"\n" "$ITEMS" "opt_sub_next" "Next Subsong"`
	    ITEMS=`printf "%s \"%s\" \"%s\"\n" "$ITEMS" "opt_sub_prev" "Previous Subsong"`
	fi
      done
      ITEMS=`printf "%s \"%s\" \"%s\"\n" "$ITEMS" "file_open" "Open File..."`
      ITEMS=`printf "%s \"%s\" \"%s\"\n" "$ITEMS" "file_m3u" "Playlist"`
      ITEMS=`printf "%s \"%s\" \"%s\"\n" "$ITEMS" "sys_exit" "Exit"`    
      
      rts=`echo -e $ITEMS|xargs $DIALOG --title "$MSG_NAME ($cat $SONGFILE)" \
          --menu "--=[ $MSG_FULLNAME ]=--
	  
	Status: $STATUS
	  
	$GUI_songinfo
	" 2>/dev/null`
        return
  
  elif [ "$1" = "open_loadfile" ]; then
     $DIALOG --title "$MSG_NAME (Open File...)" \
   --getopenfilename "$SONGPWD" > "$TEMPFILE" 1>&1 2>/dev/null
     retval=$?
  case $retval in
     0)
     rts=`cat "$TEMPFILE"`
     ;;
     1)
     rts=""
     echo "Cancel pressed.";;
     255)
     rts=""
     echo "ESC pressed.";;
   esac
   return

   elif [ "$1" = "open_plst" ];then
   	   rts=`echo -e $GUI_kdialog_plst|xargs $DIALOG --title "$MSG_NAME (M3U Playlist)" \
               --radiolist "--=[ $MSG_FULLNAME playlist ]=--

				$SONGFILE" 2>/dev/null`
     return
   
   elif [ "$1" = "build_plst" ];then
   ### prepare Playlist for kdialog
    GUI_kdialog_plst=""
    maxline=`echo -e "$MISC_plst"|wc -l |sed s/[" "]*/""/|cut -d " " -f 1`
    for ((i=0; i<= maxline; i++))
     do
      mline=`echo -e "$MISC_plst"|sed -n "$i"p`
	 mlineurl=`echo -e "$mline"|grep -v --regexp="#EXT"`
	 if [ "$mlineurl" = "" ] ; then
	    mlineext=`echo -e "$mline"|sed 's/#EXTINF://;s/"//g'`    ## save #EXTM3UInfo
	 else
	       GUI_kdialog_plst=`printf "%s\n\"%s\"\n" "$GUI_kdialog_plst" "$mlineurl"`
	     if [ "$mlineext" = "" ] ; then
	       GUI_kdialog_plst=`printf "%s\n\"#EXTINF:%s\"\n" "$GUI_kdialog_plst" "$mlineurl"`
	     else
	       GUI_kdialog_plst=`printf "%s\n\"%s\"\n" "$GUI_kdialog_plst" "$mlineext"`
	     fi
	       GUI_kdialog_plst=`printf "%s\n\"%s\"\n" "$GUI_kdialog_plst" "off"`
	  fi
     done
  fi
}
###-------------------------------------------------------------------------###
### Misc functions: playlist et al
function MISC_define_as_plst {
MISC_init_plst
MISC_add_plst_to_plst "$1"
}

function MISC_add_to_plst {
MISC_plst=`printf "%s\n#EXTINF:%s\n%s\n" "$MISC_plst" "$(basename "$1")" "$1"`
SONGFILE=`echo -e "$MISC_plst"|grep -v --regexp="#EXT"|sed -n 1p`
GUI_build_plst
}

function MISC_add_plst_to_plst {
MISC_plst=`printf "%s\n%s\n" "$MISC_plst" "$(cat $1)"`
SONGFILE=`echo -e "$MISC_plst"|grep -v --regexp="#EXT"|sed -n 1p`
GUI_build_plst
}

function MISC_init_plst {
MISC_plst="#EXTM3U"
}

###-------------------------------------------------------------------------###
### Main
###-------------------------------------------------------------------------###
### Startup sequence
 
 TRANS_available		# Check which User Interface are available
 if [ "$TRANS_list" = "" ];then
 echo "$MSG_NAME: Main: no supported transportation system  [$TRANS_list_sys] found"
 exit -1
 fi
 
 GUI_available		# Check which User Interface are available
 if [ "$GUI_list" = "" ];then
 echo "$MSG_NAME: Main: no supported user interface [$GUI_list_sys] found"
 exit -1
 fi

 GUI_is_active		# Check which User Interface is active
 if [ "$GUI" = "" ];then
 echo "$MSG_NAME: Main: none of  the supported user interfaces [$GUI_list] found"
 exit -1
 fi
 
 INP_available		# Check which Input Players are available
 if [ "$INP_list" = "" ];then
 echo "$MSG_NAME: none of the player engines [$INP_list_sys] found"
 exit -1
 fi
 
## everything running
 
 MISC_init_plst # Initialize our Playlist
 SONGFILE="$1"
 if [ "$1" = "" ]; then
     GUI_open_loadfile
       if [ "$rts" = "" ]; then
	exit 0
       fi 
 fi

  if test -z `echo "$SONGFILE" | sed -n 's/.*\.\(.*\)/\1/p'|grep 'm3u'` ; then
   TRANS_load_file "$SONGFILE"	## "download" the file
   INP_is_our_file "$SONGFILE"    ## Check if any player can play our file
   INP_set_option
   MISC_add_to_plst "$SONGFILE"
   INP_play "$SONGFILE" 
   GUI_command=""		## play first song right away, open Main window
  else
   TRANS_load_file "$SONGFILE"	## "download" the file
   INP_is_our_file "$SONGFILE"    ## Check if any player can play our file
   INP_set_option
   INP_play "$SONGFILE" 
   GUI_command="file_m3u"
  fi


##Main loop

while :
do   
 ##Parse GUI_command 
 
 ## stop song
  if [ "$GUI_command" = "song_stop" ];then
    INP_stop
  fi
    
 ## play song
  if [ "$GUI_command" = "song_play" ];then
    INP_play "$SONGFILE"   
  fi

  ## exit Player
  if [ "$GUI_command" = "sys_exit" ];then
    INP_stop
    exit 0 
  fi 
 
 ## subsongs  
  if [ "$GUI_command" = "sub_next" ];then
  echo
  fi

  if [ "$GUI_command" = "sub_prev" ];then
  echo
  fi

 ## file open   
  if [ "$GUI_command" = "file_open" ];then
    GUI_open_loadfile
       if [ "$rts" != "" ]; then
         TRANS_load_file "$SONGFILE"
         INP_is_our_file "$SONGFILE"
	 INP_set_option
         INP_play "$SONGFILE"
	 MISC_add_to_plst "$SONGFILE"
       fi

 ## options   
  elif [ "$GUI_command" = "opt_ntsc" ];then
  	INP_set_option ntsc 
  	INP_play "$SONGFILE"  

  elif [ "$GUI_command" = "opt_pan" ];then
  	INP_set_option pan
  	INP_play "$SONGFILE"

  elif [ "$GUI_command" = "opt_sub_next" ];then
  	INP_set_option sub_next
  	INP_play "$SONGFILE"
  
  elif [ "$GUI_command" = "opt_sub_prev" ];then
  	INP_set_option sub_prev
  	INP_play "$SONGFILE"
  fi

  if [ "$GUI_command" = "file_m3u" ];then
	GUI_open_plst
	 if [ "$rts" != "" ]; then
          TRANS_load_file "$SONGFILE"
           INP_is_our_file "$SONGFILE"
	   INP_set_option
           INP_play "$SONGFILE"
	 else
	   GUI_command=""
	 fi
  else
	GUI_open_main
  fi
done
return 0