/* XMMS UADE plugin
 *
 * Copyright (C) 2000-2003  Heikki Orsila
 *                          heikki.orsila@iki.fi
 *                          http://uade.ton.tut.fi
 *
 * This plugin is based on xmms 0.9.6 wavplayer input plugin code. Since
 * then all code has been rewritten.
 *
 * the intital gui code was based onthe null-plugin by
 * Håvard Kvålen. Since then the gui code has evolved and was also mostly
 * rewritten.
 * Formatseditor inspired by nscache 0.3's mimetype editor by Stefan Ondrejicka
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <glib.h>
#include <gtk/gtk.h>
#include <xmms/plugin.h>
#include <xmms/configfile.h>
#include <xmms/util.h>

#include "uade.h"
#include "uade-os.h"
#include "uadeconfig.h"
#include "gui.h"
#include "dpiutil.h"

#include "../osdep/strl.c"

GtkWidget *fileinfowin = NULL;
GtkWidget *playerinfowin = NULL;
GtkWidget *modinfowin = NULL;

GtkWidget *fileinfo_modulename_txt;
//GtkWidget *fileinfo_modulepath_txt;
GtkWidget *fileinfo_playername_txt;
GtkWidget *fileinfo_maxsubsong_txt;
GtkWidget *fileinfo_minsubsong_txt;
GtkWidget *fileinfo_subsong_txt;

/* File Info Window */


void uade_fileinfo(void)
{
    GtkWidget *fileinfo_base_vbox;
    GtkWidget *fileinfo_frame;
    GtkWidget *fileinfo_table;

    GtkWidget *fileinfo_modulename_label;
//  GtkWidget *fileinfo_modulepath_label;
    GtkWidget *fileinfo_moduleinfo_button;
    GtkWidget *fileinfo_modulename_hbox;
    GtkWidget *fileinfo_hrule1;
    GtkWidget *fileinfo_playername_hbox;
    GtkWidget *fileinfo_playername_label;
    GtkWidget *fileinfo_playerinfo_button;
    GtkWidget *fileinfo_hrule2;
    GtkWidget *fileinfo_subsong_label;
    GtkWidget *fileinfo_minsubsong_label;
    GtkWidget *fileinfo_maxsubsong_label;
    GtkWidget *fileinfo_hrule3;
    GtkWidget *fileinfo_hrule4;

    GtkWidget *fileinfo_button_box;
    GtkWidget *ok_button;

    GtkTooltips *fileinfo_tooltips;

    char *playername;
    char *formatname;

    if (uade_song_basename) {

	if (!fileinfowin) {

	    fileinfo_tooltips = gtk_tooltips_new();

	    fileinfowin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	    gtk_window_set_title(GTK_WINDOW(fileinfowin), "UADE Fileinfo");
	    gtk_window_set_position(GTK_WINDOW(fileinfowin),
				    GTK_WIN_POS_MOUSE);
	    gtk_container_set_border_width(GTK_CONTAINER(fileinfowin), 10);
	    gtk_window_set_policy(GTK_WINDOW(fileinfowin), FALSE, FALSE,
				  FALSE);
	    gtk_signal_connect(GTK_OBJECT(fileinfowin), "destroy",
			       GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			       &fileinfowin);

//Start of Contents Box

	    fileinfo_base_vbox = gtk_vbox_new(FALSE, 10);

	    gtk_container_set_border_width(GTK_CONTAINER
					   (fileinfo_base_vbox), 5);
	    gtk_container_add(GTK_CONTAINER(fileinfowin),
			      fileinfo_base_vbox);


//Start of FileInfo frame, text and option widgets

	    fileinfo_frame = gtk_frame_new("FileInfo: ");
	    gtk_box_pack_start(GTK_BOX(fileinfo_base_vbox), fileinfo_frame,
			       TRUE, TRUE, 0);


/*Start of Fileinfotable */

	    fileinfo_table = gtk_table_new(12, 2, FALSE);

	    gtk_widget_show(fileinfo_table);
	    gtk_container_add(GTK_CONTAINER(fileinfo_frame),
			      fileinfo_table);
	    gtk_container_set_border_width(GTK_CONTAINER(fileinfo_table),
					   5);

/* 1x1 */

	    fileinfo_modulename_label = gtk_label_new("Module: ");
	    gtk_misc_set_padding(GTK_MISC(fileinfo_modulename_label), 5,
				 5);
	    //gtk_misc_set_alignment (GTK_MISC (fileinfo_modulename_label), 1, 0);                          
	    gtk_table_attach(GTK_TABLE(fileinfo_table),
			     fileinfo_modulename_label, 0, 1, 0, 1,
			     (GtkAttachOptions) (GTK_FILL),
			     (GtkAttachOptions) (GTK_FILL), 0, 0);

/* 1x2 */
/*
      fileinfo_modulepath_label = gtk_label_new("module path: ");
      gtk_misc_set_padding(GTK_MISC(fileinfo_modulepath_label), 5, 5);
      //gtk_misc_set_alignment (GTK_MISC (fileinfo_modulepath_label), 1, 0);                          
      gtk_table_attach(GTK_TABLE(fileinfo_table),
		       fileinfo_modulepath_label, 0, 1, 1, 2,
		       (GtkAttachOptions) (GTK_FILL),
		       (GtkAttachOptions) (GTK_FILL), 0, 0);
*/
/* 1x3 */

	    fileinfo_hrule1 = gtk_hseparator_new();
	    gtk_table_attach(GTK_TABLE(fileinfo_table), fileinfo_hrule1, 0,
			     1, 2, 3, (GtkAttachOptions) (GTK_FILL),
			     (GtkAttachOptions) (0), 0, 0);

/* 1x4 */

	    fileinfo_playername_label = gtk_label_new("Playerformat: ");
	    gtk_misc_set_padding(GTK_MISC(fileinfo_playername_label), 5,
				 5);
	    //gtk_misc_set_alignment (GTK_MISC (fileinfo_playername_label), 1, 0);                          
	    gtk_table_attach(GTK_TABLE(fileinfo_table),
			     fileinfo_playername_label, 0, 1, 3, 4,
			     (GtkAttachOptions) (GTK_FILL),
			     (GtkAttachOptions) (GTK_FILL), 0, 0);


	    fileinfo_hrule2 = gtk_hseparator_new();
	    gtk_table_attach(GTK_TABLE(fileinfo_table), fileinfo_hrule2, 0,
			     1, 6, 7, (GtkAttachOptions) (GTK_FILL),
			     (GtkAttachOptions) (0), 0, 0);

/* 1x8*/

	    fileinfo_subsong_label = gtk_label_new("Curr. subsong: ");
	    gtk_misc_set_padding(GTK_MISC(fileinfo_subsong_label), 5, 5);
	    //gtk_misc_set_alignment (GTK_MISC (fileinfo_subsong_label), 1, 0);                          
	    gtk_table_attach(GTK_TABLE(fileinfo_table),
			     fileinfo_subsong_label, 0, 1, 7, 8,
			     (GtkAttachOptions) (GTK_FILL),
			     (GtkAttachOptions) (GTK_FILL), 0, 0);

/* 1x9*/

	    fileinfo_minsubsong_label = gtk_label_new("Min. subsong: ");
	    gtk_misc_set_padding(GTK_MISC(fileinfo_minsubsong_label), 5,
				 5);
	    //gtk_misc_set_alignment (GTK_MISC (fileinfo_minsubsong_label), 1, 0);                          
	    gtk_table_attach(GTK_TABLE(fileinfo_table),
			     fileinfo_minsubsong_label, 0, 1, 8, 9,
			     (GtkAttachOptions) (GTK_FILL),
			     (GtkAttachOptions) (GTK_FILL), 0, 0);

/* 1x10*/

	    fileinfo_maxsubsong_label = gtk_label_new("Max. subsong: ");
	    gtk_misc_set_padding(GTK_MISC(fileinfo_maxsubsong_label), 5,
				 5);
	    //gtk_misc_set_alignment (GTK_MISC (fileinfo_maxsubsong_label), 1, 0);                          
	    gtk_table_attach(GTK_TABLE(fileinfo_table),
			     fileinfo_maxsubsong_label, 0, 1, 9, 10,
			     (GtkAttachOptions) (GTK_FILL),
			     (GtkAttachOptions) (GTK_FILL), 0, 0);

/* 2nd Column */
/* 2x1*/
	    fileinfo_modulename_hbox = gtk_hbox_new(FALSE, 10);
	    gtk_table_attach(GTK_TABLE(fileinfo_table),
			     fileinfo_modulename_hbox, 1, 2, 0, 1,
			     (GtkAttachOptions) (GTK_FILL),
			     (GtkAttachOptions) (0), 0, 0);

	    fileinfo_modulename_txt = gtk_label_new(get_modulename());
	    gtk_label_set_justify(GTK_LABEL(fileinfo_modulename_txt),
				  GTK_JUSTIFY_LEFT);
	    gtk_label_set_line_wrap(GTK_LABEL(fileinfo_modulename_txt),
				    TRUE);
	    gtk_misc_set_alignment(GTK_MISC(fileinfo_modulename_txt), 0,
				   0.5);
	    gtk_misc_set_padding(GTK_MISC(fileinfo_modulename_txt), 5, 5);

	    fileinfo_moduleinfo_button = gtk_button_new_with_label("?");
	    GTK_WIDGET_SET_FLAGS(fileinfo_moduleinfo_button,
				 GTK_CAN_DEFAULT);

	    gtk_widget_ref(fileinfo_moduleinfo_button);
	    gtk_object_set_data_full(GTK_OBJECT(fileinfowin),
				     "fileinfo_moduleinfo_button",
				     fileinfo_moduleinfo_button,
				     (GtkDestroyNotify) gtk_widget_unref);
	    gtk_tooltips_set_tip(fileinfo_tooltips,
				 fileinfo_moduleinfo_button,
				 g_strdup_printf("%s",
						 uade_song_full_name),
				 NULL);


	    gtk_signal_connect_object(GTK_OBJECT
				      (fileinfo_moduleinfo_button),
				      "clicked",
				      GTK_SIGNAL_FUNC(uade_modinfo), NULL);


	    gtk_box_pack_start(GTK_BOX(fileinfo_modulename_hbox),
			       fileinfo_modulename_txt, TRUE, TRUE, 0);
	    gtk_box_pack_start_defaults(GTK_BOX(fileinfo_modulename_hbox),
					fileinfo_moduleinfo_button);


/* 2x2*/
/*
      fileinfo_modulepath_txt = gtk_label_new(fileinfo_modulepath);
      gtk_table_attach(GTK_TABLE(fileinfo_table),
		       fileinfo_modulepath_txt, 1, 2, 1, 2,
		       (GtkAttachOptions) (GTK_FILL),
		       (GtkAttachOptions) (0), 0, 0);
      gtk_label_set_justify(GTK_LABEL(fileinfo_modulepath_txt),
			    GTK_JUSTIFY_CENTER);
      gtk_label_set_line_wrap(GTK_LABEL(fileinfo_modulepath_txt), TRUE);
      gtk_misc_set_alignment(GTK_MISC(fileinfo_modulepath_txt), 0, 0.5);
      gtk_misc_set_padding(GTK_MISC(fileinfo_modulepath_txt), 5, 5);

	gtk_widget_ref (fileinfo_modulepath_txt);
	gtk_object_set_data_full (GTK_OBJECT(fileinfowin), "fileinfo_modulepath_txt",
				    fileinfo_modulepath_txt,
				    (GtkDestroyNotify) gtk_widget_unref);
	gtk_tooltips_set_tip (fileinfo_tooltips, fileinfo_modulepath_txt, uade_song_full_name, NULL);

*/

/* 2x3*/
	    fileinfo_hrule3 = gtk_hseparator_new();
	    gtk_table_attach(GTK_TABLE(fileinfo_table), fileinfo_hrule3, 1,
			     2, 2, 3, (GtkAttachOptions) (GTK_FILL),
			     (GtkAttachOptions) (0), 0, 0);


/* 2x4*/

	    fileinfo_playername_hbox = gtk_hbox_new(FALSE, 10);
	    gtk_table_attach(GTK_TABLE(fileinfo_table),
			     fileinfo_playername_hbox, 1, 2, 3, 4,
			     (GtkAttachOptions) (GTK_FILL),
			     (GtkAttachOptions) (0), 0, 0);

	    formatname = get_formatname();
	    playername = get_playername();
	    if (formatname[0] == 0) {
		fileinfo_playername_txt =
		    gtk_label_new(g_strdup_printf("%s", playername));
	    } else {
		/* memory leaks using g_strdup_printf? */
		fileinfo_playername_txt =
		    gtk_label_new(g_strdup_printf
				  ("%s\n%s", playername, formatname));
	    }
	    //fileinfo_playername_txt = gtk_label_new (get_playername());
	    gtk_label_set_justify(GTK_LABEL(fileinfo_playername_txt),
				  GTK_JUSTIFY_LEFT);
	    gtk_label_set_line_wrap(GTK_LABEL(fileinfo_playername_txt),
				    TRUE);
	    gtk_misc_set_alignment(GTK_MISC(fileinfo_playername_txt), 0,
				   0.5);
	    gtk_misc_set_padding(GTK_MISC(fileinfo_playername_txt), 5, 5);

	    fileinfo_playerinfo_button = gtk_button_new_with_label("?");
	    GTK_WIDGET_SET_FLAGS(fileinfo_playerinfo_button,
				 GTK_CAN_DEFAULT);
	    gtk_signal_connect_object(GTK_OBJECT
				      (fileinfo_playerinfo_button),
				      "clicked",
				      GTK_SIGNAL_FUNC(uade_playerinfo),
				      NULL);


	    gtk_box_pack_start(GTK_BOX(fileinfo_playername_hbox),
			       fileinfo_playername_txt, TRUE, TRUE, 0);
	    gtk_box_pack_start_defaults(GTK_BOX(fileinfo_playername_hbox),
					fileinfo_playerinfo_button);


	    fileinfo_hrule4 = gtk_hseparator_new();
	    gtk_table_attach(GTK_TABLE(fileinfo_table), fileinfo_hrule4, 1,
			     2, 6, 7, (GtkAttachOptions) (GTK_FILL),
			     (GtkAttachOptions) (0), 0, 0);


	    fileinfo_subsong_txt =
		gtk_label_new(g_strdup_printf("%d", get_curr_subsong()));

	    /* gtk_widget_setusize for this widget is a bit of a cludge to set 
	       a minimal size for the FileinfoWindow. I can't get it to work either with
	       setting the usize for the window or the table... weird */

	    gtk_widget_set_usize(fileinfo_subsong_txt, 176, -2);


	    gtk_table_attach(GTK_TABLE(fileinfo_table),
			     fileinfo_subsong_txt, 1, 2, 7, 8,
			     (GtkAttachOptions) (GTK_FILL),
			     (GtkAttachOptions) (0), 0, 0);
	    gtk_label_set_justify(GTK_LABEL(fileinfo_subsong_txt),
				  GTK_JUSTIFY_LEFT);
	    gtk_label_set_line_wrap(GTK_LABEL(fileinfo_subsong_txt), TRUE);
	    gtk_misc_set_alignment(GTK_MISC(fileinfo_subsong_txt), 0, 0.5);
	    gtk_misc_set_padding(GTK_MISC(fileinfo_subsong_txt), 5, 5);

/* 2x9*/
	    fileinfo_minsubsong_txt =
		gtk_label_new(g_strdup_printf("%d", get_min_subsong()));
	    gtk_table_attach(GTK_TABLE(fileinfo_table),
			     fileinfo_minsubsong_txt, 1, 2, 8, 9,
			     (GtkAttachOptions) (GTK_FILL),
			     (GtkAttachOptions) (0), 0, 0);
	    gtk_label_set_justify(GTK_LABEL(fileinfo_minsubsong_txt),
				  GTK_JUSTIFY_LEFT);
	    gtk_label_set_line_wrap(GTK_LABEL(fileinfo_subsong_txt), TRUE);
	    gtk_misc_set_alignment(GTK_MISC(fileinfo_minsubsong_txt), 0,
				   0.5);
	    gtk_misc_set_padding(GTK_MISC(fileinfo_minsubsong_txt), 5, 5);

/* 2x10*/
	    fileinfo_maxsubsong_txt =
		gtk_label_new(g_strdup_printf("%d", get_max_subsong()));
	    gtk_table_attach(GTK_TABLE(fileinfo_table),
			     fileinfo_maxsubsong_txt, 1, 2, 9, 10,
			     (GtkAttachOptions) (GTK_FILL),
			     (GtkAttachOptions) (0), 0, 0);
	    gtk_label_set_justify(GTK_LABEL(fileinfo_maxsubsong_txt),
				  GTK_JUSTIFY_LEFT);
	    gtk_label_set_line_wrap(GTK_LABEL(fileinfo_maxsubsong_txt),
				    TRUE);
	    gtk_misc_set_alignment(GTK_MISC(fileinfo_maxsubsong_txt), 0,
				   0.5);
	    gtk_misc_set_padding(GTK_MISC(fileinfo_maxsubsong_txt), 5, 5);

// end of frame.


// Start of Ok and Cancel Button Box

	    fileinfo_button_box = gtk_hbutton_box_new();


	    gtk_button_box_set_layout(GTK_BUTTON_BOX(fileinfo_button_box),
				      GTK_BUTTONBOX_END);
	    gtk_button_box_set_spacing(GTK_BUTTON_BOX(fileinfo_button_box),
				       5);
	    gtk_box_pack_start(GTK_BOX(fileinfo_base_vbox),
			       fileinfo_button_box, FALSE, FALSE, 0);


	    ok_button = gtk_button_new_with_label("Close");
	    GTK_WIDGET_SET_FLAGS(ok_button, GTK_CAN_DEFAULT);
	    gtk_signal_connect_object(GTK_OBJECT(ok_button), "clicked",
				      GTK_SIGNAL_FUNC(gtk_widget_destroy),
				      GTK_OBJECT(fileinfowin));

	    gtk_box_pack_start_defaults(GTK_BOX(fileinfo_button_box),
					ok_button);

// End of Button Box

	    gtk_widget_show_all(fileinfowin);

	} else {
	    //fprintf (stderr,"updating fileinfo\n");
	    fileinfo_update();
	}
    }
}

void fileinfo_update()
{
    char *playername;
    char *formatname;

    gdk_window_raise(fileinfowin->window);

    gtk_label_set_text(GTK_LABEL(fileinfo_modulename_txt),
		       g_strdup_printf("%s", get_modulename()));
    gtk_widget_show(fileinfo_modulename_txt);

/*  gtk_label_set_text(GTK_LABEL(fileinfo_modulepath_txt),
		     g_strdup_printf("%s", fileinfo_modulepath));
  gtk_widget_show(fileinfo_modulepath_txt);
*/

    formatname = get_formatname();
    playername = get_playername();
    if (formatname[0] == 0) {
	gtk_label_set_text(GTK_LABEL(fileinfo_playername_txt),
			   g_strdup_printf("%s", playername));
    } else {
	gtk_label_set_text(GTK_LABEL(fileinfo_playername_txt),
			   g_strdup_printf("%s\n%s", playername,
					   formatname));

    }

    gtk_widget_show(fileinfo_playername_txt);
    gtk_label_set_text(GTK_LABEL(fileinfo_subsong_txt),
		       g_strdup_printf("%d", get_curr_subsong()));
    gtk_widget_show(fileinfo_subsong_txt);

    gtk_label_set_text(GTK_LABEL(fileinfo_minsubsong_txt),
		       g_strdup_printf("%d", get_min_subsong()));
    gtk_widget_show(fileinfo_minsubsong_txt);

    gtk_label_set_text(GTK_LABEL(fileinfo_maxsubsong_txt),
		       g_strdup_printf("%d", get_max_subsong()));
    gtk_widget_show(fileinfo_maxsubsong_txt);

}

void uade_playerinfo(void)
{

    char player_filename[1024] = "";
    char credits[4096] = "";
    GtkWidget *playerinfo_button_box;
    GtkWidget *close_button;
    GtkWidget *playerinfo_base_vbox;

    GtkWidget *uadeplay_scrolledwindow;
    GtkWidget *uadeplay_txt;


    if (!playerinfowin) {


	playerinfowin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(playerinfowin), "UADE Playerinfo");
	gtk_window_set_position(GTK_WINDOW(playerinfowin),
				GTK_WIN_POS_MOUSE);
	gtk_container_set_border_width(GTK_CONTAINER(playerinfowin), 10);
	gtk_window_set_policy(GTK_WINDOW(playerinfowin), FALSE, FALSE,
			      FALSE);
	gtk_signal_connect(GTK_OBJECT(playerinfowin), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			   &playerinfowin);
//Start of Contents Box

	playerinfo_base_vbox = gtk_vbox_new(FALSE, 10);

	gtk_container_set_border_width(GTK_CONTAINER(playerinfo_base_vbox),
				       5);
	gtk_container_add(GTK_CONTAINER(playerinfowin),
			  playerinfo_base_vbox);


	strlcpy(player_filename, get_playerfilename(),
		sizeof(player_filename));
	process_eagleplayer(credits, player_filename, sizeof(credits));

	uadeplay_scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(playerinfo_base_vbox),
			  uadeplay_scrolledwindow);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW
				       (uadeplay_scrolledwindow),
				       GTK_POLICY_NEVER,
				       GTK_POLICY_ALWAYS);

	uadeplay_txt = gtk_text_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(uadeplay_scrolledwindow),
			  uadeplay_txt);
	gtk_text_insert(GTK_TEXT(uadeplay_txt), NULL, NULL, NULL, credits,
			-1);

	gtk_text_set_word_wrap(GTK_TEXT(uadeplay_txt), TRUE);
	gtk_widget_set_usize(uadeplay_scrolledwindow, 400, 240);


// Start of Close Button Box

	playerinfo_button_box = gtk_hbutton_box_new();

	gtk_button_box_set_layout(GTK_BUTTON_BOX(playerinfo_button_box),
				  GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(playerinfo_button_box),
				   5);
	gtk_box_pack_start(GTK_BOX(playerinfo_base_vbox),
			   playerinfo_button_box, FALSE, FALSE, 0);

	close_button = gtk_button_new_with_label("Close");
	GTK_WIDGET_SET_FLAGS(close_button, GTK_CAN_DEFAULT);
	gtk_signal_connect_object(GTK_OBJECT(close_button), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(playerinfowin));

	gtk_box_pack_start_defaults(GTK_BOX(playerinfo_button_box),
				    close_button);
	gtk_widget_show_all(playerinfowin);
    } else {
	gdk_window_raise(playerinfowin->window);
    }

}

void uade_modinfo(void)
{

    char mod_filename[1024] = "\0";
    char credits[2048] = "\0";
    GtkWidget *modinfo_button_box;
    GtkWidget *close_button;
    GtkWidget *modinfo_base_vbox;

    GtkWidget *uadeplay_scrolledwindow;
    GtkWidget *uadeplay_txt;


    if (!modinfowin) {


	modinfowin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(modinfowin), "UADE Modinfo");
	gtk_window_set_position(GTK_WINDOW(modinfowin), GTK_WIN_POS_MOUSE);
	gtk_container_set_border_width(GTK_CONTAINER(modinfowin), 10);
	gtk_window_set_policy(GTK_WINDOW(modinfowin), FALSE, FALSE, FALSE);
	gtk_signal_connect(GTK_OBJECT(modinfowin), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			   &modinfowin);
//Start of Contents Box

	modinfo_base_vbox = gtk_vbox_new(FALSE, 10);

	gtk_container_set_border_width(GTK_CONTAINER(modinfo_base_vbox),
				       5);
	gtk_container_add(GTK_CONTAINER(modinfowin), modinfo_base_vbox);


	strlcpy(mod_filename, get_modulefilename(), sizeof(mod_filename));
	processmodule(credits, mod_filename, sizeof(credits));

	uadeplay_scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(modinfo_base_vbox),
			  uadeplay_scrolledwindow);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW
				       (uadeplay_scrolledwindow),
				       GTK_POLICY_NEVER,
				       GTK_POLICY_ALWAYS);

	uadeplay_txt = gtk_text_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(uadeplay_scrolledwindow),
			  uadeplay_txt);
	gtk_text_insert(GTK_TEXT(uadeplay_txt), NULL, NULL, NULL, credits,
			-1);

	gtk_text_set_word_wrap(GTK_TEXT(uadeplay_txt), TRUE);
	//gtk_widget_set_usize(uadeplay_scrolledwindow, 400, 240);
	gtk_widget_set_usize(uadeplay_scrolledwindow, 600, 240);


// Start of Close Button Box

	modinfo_button_box = gtk_hbutton_box_new();

	gtk_button_box_set_layout(GTK_BUTTON_BOX(modinfo_button_box),
				  GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(modinfo_button_box), 5);
	gtk_box_pack_start(GTK_BOX(modinfo_base_vbox), modinfo_button_box,
			   FALSE, FALSE, 0);

	close_button = gtk_button_new_with_label("Close");
	GTK_WIDGET_SET_FLAGS(close_button, GTK_CAN_DEFAULT);
	gtk_signal_connect_object(GTK_OBJECT(close_button), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(modinfowin));

	gtk_box_pack_start_defaults(GTK_BOX(modinfo_button_box),
				    close_button);
	gtk_widget_show_all(modinfowin);
    } else {
	gdk_window_raise(modinfowin->window);
    }

}
