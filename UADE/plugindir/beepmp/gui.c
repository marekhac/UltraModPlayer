/*
 * $Id:gui.c
 * 
 * UADE plugin for Beep Media Player
 *
 * This is a "general" Beep Media Player plugin (that is, one that doesn't
 * provide any audio processing functions).  When enabled, it grabs the
 * XF86Audio* keys for play/stop/next/previous, then translates keyrelease
 * events on those keys into Beep Media Player actions.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * Copyright (c) 2004 mld /uade team
 *
 */

#include <bmp/plugin.h>
#include <bmp/util.h>
#include <bmp/beepctrl.h>
#include <gtk/gtk.h>

#include "gui.h"
#include "../../osdep/strl.c"
#include "uade.h"

/* plugin_about(): called when the "about" button in the plugins
 * configuration is pressed.  Typical behavior here is to present
 * a dialog explaining what the plugin does.
 */
void uade_about()
{
	static GtkWidget *about;

	if (about != NULL)
		return;

	about = xmms_show_message(
			"About UADE Input plugin",
			"Unix Amiga Delitracker Input Plugin " VERSION "\n\n"
			  "This plugin enables Beep Media Player to play Amiga music files\n"
			  "like MOD, MED, TFMX, DW, RH and a lot of others too.\n\n"
			  "Copyright (c) 2004 by Heikki 'shd' Orsila & Michael 'mld' Doering\n"
			  "http://uade.ton.tut.fi\n\n"
			  "This plugin is free software, released under the terms of the GNU\n"
			  "General Public License, v2.  You should have received a copy of\n"
			  "the license with this software.\n",
			"OK", 1, NULL, NULL);
	gtk_signal_connect(GTK_OBJECT(about), "destroy",
			GTK_SIGNAL_FUNC(gtk_widget_destroyed), &about);
	gtk_widget_show (about);

}
/*
void uade_configure()
{
	static GtkWidget *configure;

	if (configure != NULL)
		return;

	configure = xmms_show_message(
			"Configuring UADE Input plugin",
			"Unix Amiga Delitracker Input Plugin " VERSION "\n\n"
			 "Sorry, there's no real GUI code for\n"
			 "configuring ntsc, filters etc for\n"
			 "this BeepMP Input Plugin, yet.\n"
			 "(check: defaults.c)\n"
			 "\n"
			 "You can however already switch subsongs\n"
			 "by focussing the BeepMP Main window and\n"
			 "using the  '<-' '->'  arrow keys on your\n"
			 "keyboard.\n",
			"OK", 1, NULL, NULL);
	gtk_signal_connect(GTK_OBJECT(configure), "destroy",
			GTK_SIGNAL_FUNC(gtk_widget_destroyed), &configure);

}
*/

/* plugin_modinfo(): called when the "about" button in the plugins
 * configuration is pressed.  Typical behavior here is to present
 * a dialog explaining what the plugin does.
 */
void uade_modinfo()
{
	
	char mod_filename[1024] = "\0";
	char credits[2048] ="\0";
	static GtkWidget *modinfo;


	strlcpy(mod_filename, get_modulefilename(), sizeof(mod_filename));
	processmodule(credits, mod_filename, sizeof(credits));


	if (modinfo != NULL)
		return;

	modinfo = xmms_show_message(
			"UADE Modinfo",
			credits,
			"OK", FALSE, NULL, NULL);
	gtk_signal_connect(GTK_OBJECT(modinfo), "destroy",
			GTK_SIGNAL_FUNC(gtk_widget_destroyed), &modinfo);
	gtk_widget_show (modinfo);
}
