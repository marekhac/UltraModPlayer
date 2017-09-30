UADE Launcher/loader scripts:

for people that don't want to mess around with the console, there's some 
simple scripts to launch and exit UADE from an graphical enviroment.

gnome:
    provides:		an "Open filename" and "Playing" window.

    requires:		"gdialog" from the gnome-utils package, UADE compiled with SDL

    installation:	make a shortcut to the launcher script on your desktop
			or copy both file the desktop entry and the script to
			a place of your choice

    usage:		either start the shortcut, and enter the filename 
	    		or drag'n drop that file from Nautilus onto that
			shortcut.

    optional:		if you compiled uade with plain OSS output and you'r using ESound,
			you have to edit the script and call "esddsp $HOME/.xmms/uade/uade" instead of
			"$HOME/.xmms/uade/uade"
			
kde:
    provides:		an "Open filename" and "Playing" window.

    requires:		"kdialog" from the kdebase package, UADE compiled with SDL

    installation:	make a shortcut to the launcher script on your desktop
			or copy both file the desktop entry and the script to
			a place of your choice

    usage:		either start the shortcut, and enter the filename 
	    		or drag'n drop that file from Konqueror onto that
			shortcut.

    optional:		if you compiled uade with plain OSS output, and you are using ARTS on KDE,
	                you have to edit the script and call "artsdsp $HOME/.xmms/uade/uade" instead of
			"$HOME/.xmms/uade/uade"
			

info: any application that supports the gnome/kde DnD protocol should work with
      those script as well. (rox, ark)

Tested on RH9, Gnome 2.2, KDE 3.1 with UADE 0.82 compiled with SDL Audio, so it chooses automatically the
right sound output (ALSA, OSS, ESD, ARTS).
    
Credits: Inspired by the mos-loader script by Dave Crawford.

have fun, no warranty. use at own risk.

uade team