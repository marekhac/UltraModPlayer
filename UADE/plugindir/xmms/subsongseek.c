/* XMMS UADE plugin
 *
 * Copyright (C) 2000-2004  Heikki Orsila
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

#include "defaults.h"
#include "uade.h"
#include "gui.h"

GtkWidget *seekpopup = NULL;
GtkObject *subsong_adj;
GtkWidget *ntsc_switch;

extern InputPlugin uade_ip;

static void uade_seek_directly(void);
static void uade_seek_next(void);
static void uade_seek_previous(void);
static void focus_out_event();
static void uade_seek_update_display(int subsong);
static void uade_switch_ntsc();

/* if seek_next or seek_previous returns -1 then subsong is not changed */
static int get_next_subsong(void)
{
    int newsubsong;
    if (!is_paused()) {
	newsubsong = get_curr_subsong();
	newsubsong++;
	return newsubsong;
    } else {
	return -1;
    }
}

static int get_previous_subsong(void)
{
    int newsubsong;
    if (!is_paused()) {
	newsubsong = get_curr_subsong();
	if (newsubsong > get_min_subsong()) {
	    newsubsong--;
	    return newsubsong;
	}
    }
    return -1;
}

/* popup for seeking to a subsong*/

void uade_seeksubsong(int to)
{
    GtkWidget *seek_button_box;
    GtkWidget *seek_button_vbox;
    GtkWidget *seek_slider_box;

    GtkWidget *prev_button, *prev2_button;
    GtkWidget *prev_button_frame;
    GtkWidget *frame;
    GtkWidget *hscale;
    GtkWidget *maxsong_label;
    GtkWidget *next_button, *next2_button;
    GtkWidget *next_button_frame;

    int current, subsong;

    if (!uade_song_basename) {
	fprintf(stderr,
		"uade: BUG! one shouldn't try to seek when a song is ");
	fprintf(stderr, "not loaded\n");
	return;
    }

    if (use_xmms_slider) {
	/* xmms slider usage to change subsongs by Mikael Bouillot */
	current = uade_ip.get_time() / 1000;
	subsong =
	    (to > current) ? get_next_subsong() : get_previous_subsong();
	if (subsong != -1) {
	    seek(subsong, "user request");
	    if (fileinfowin)
		fileinfo_update();
	}
	return;
    } else if (!seekpopup) {

	/* uade's subsong popup */

	seekpopup = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_window_set_title(GTK_WINDOW(seekpopup), "UADE seek subsong");
	gtk_window_set_position(GTK_WINDOW(seekpopup), GTK_WIN_POS_MOUSE);
	gtk_container_set_border_width(GTK_CONTAINER(seekpopup), 0);

	gtk_window_set_policy(GTK_WINDOW(seekpopup), FALSE, FALSE, FALSE);

	gtk_signal_connect(GTK_OBJECT(seekpopup), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			   &seekpopup);

	gtk_signal_connect(GTK_OBJECT(seekpopup), "focus_out_event",
			   GTK_SIGNAL_FUNC(focus_out_event), NULL);


	gtk_widget_realize(seekpopup);
	gdk_window_set_decorations(seekpopup->window, 0);


/* define Slider code, will be used by all styles of the popup*/

	if (get_max_subsong() > 0) {

	    subsong_adj =
		gtk_adjustment_new(get_curr_subsong(), get_min_subsong(),
				   get_max_subsong(), 1, 0, 0);	/*our scale for the subsong slider */
	    maxsong_label =
		gtk_label_new(g_strdup_printf("%d", get_max_subsong()));	/* until we can't get the reliable maximum number
										   of subsongs this has to do :-) */
	    gtk_widget_set_usize(maxsong_label, 24, -1);

	} else {
	    subsong_adj =
		gtk_adjustment_new(get_curr_subsong(), get_min_subsong(),
				   (get_min_subsong()) + 10, 1, 0, 0);	/*our scale for the subsong slider */
	    /*currently: min - min+10  */
	    maxsong_label = gtk_label_new("...");	/* until we can't get the reliable maximum number
							   of subsongs this has to do :-) */
	    gtk_widget_set_usize(maxsong_label, 24, -1);

	    //fprintf (stderr, "Curr: %d, Min: %d, Max: %d", get_curr_subsong(), get_min_subsong(), get_max_subsong());
	}

	hscale = gtk_hscale_new(GTK_ADJUSTMENT(subsong_adj));
	gtk_widget_set_usize(hscale, 160, -1);
	gtk_scale_set_digits(GTK_SCALE(hscale), 0);
	gtk_scale_set_value_pos(GTK_SCALE(hscale), GTK_POS_LEFT);
	gtk_scale_set_draw_value(GTK_SCALE(hscale), TRUE);
	gtk_range_set_update_policy(GTK_RANGE(hscale),
				    GTK_UPDATE_DISCONTINUOUS);
	gtk_signal_connect_object(GTK_OBJECT(subsong_adj), "value_changed",
				  GTK_SIGNAL_FUNC(uade_seek_directly),
				  NULL);


/* previous subsong button, will be used by all styles of the seek popup*/
	prev_button = gtk_button_new_with_label("<");
	gtk_widget_set_usize(prev_button, 27, -1);
	gtk_signal_connect_object(GTK_OBJECT(prev_button), "clicked",
				  GTK_SIGNAL_FUNC(uade_seek_previous),
				  NULL);

	prev_button_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(prev_button_frame),
				  GTK_SHADOW_IN);


	prev2_button = gtk_button_new_with_label("<");
	gtk_widget_set_usize(prev2_button, 27, -1);
	gtk_signal_connect_object(GTK_OBJECT(prev2_button), "clicked",
				  GTK_SIGNAL_FUNC(uade_seek_previous),
				  NULL);

/* next subsong button, will be used by all styles of the seek popup*/

	next_button = gtk_button_new_with_label(">");
	gtk_widget_set_usize(next_button, 27, -1);
	gtk_signal_connect_object(GTK_OBJECT(next_button), "clicked",
				  GTK_SIGNAL_FUNC(uade_seek_next), NULL);

	next_button_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(next_button_frame),
				  GTK_SHADOW_IN);

	next2_button = gtk_button_new_with_label(">");
	gtk_widget_set_usize(next2_button, 27, -1);
	gtk_signal_connect_object(GTK_OBJECT(next2_button), "clicked",
				  GTK_SIGNAL_FUNC(uade_seek_next), NULL);

	ntsc_switch = gtk_check_button_new_with_label("ntsc");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ntsc_switch),
				     use_ntsc);
	gtk_signal_connect_object(GTK_OBJECT(ntsc_switch), "clicked",
				  GTK_SIGNAL_FUNC(uade_switch_ntsc), NULL);

	/* with the alternative styles of the subsongseeker,
	 * following suggestions made by David Le Corfec*/

	seek_button_box = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(seekpopup), seek_button_box);

	if (lr_subsong_arrows == TRUE) {
	    gtk_box_pack_start_defaults(GTK_BOX(seek_button_box),
					prev2_button);
	}



	frame = gtk_frame_new(NULL);
	gtk_box_pack_start_defaults(GTK_BOX(seek_button_box), frame);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);


	seek_button_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), seek_button_vbox);
	gtk_signal_connect(GTK_OBJECT(seek_button_vbox), "focus_out_event",
			   GTK_SIGNAL_FUNC(focus_out_event), NULL);

	if (ud_subsong_arrows == TRUE) {
	    /* use the previous defined buttons here */
	    gtk_box_pack_start_defaults(GTK_BOX(seek_button_vbox),
					prev_button_frame);
	    gtk_container_add(GTK_CONTAINER(prev_button_frame),
			      prev_button);
	}


	seek_slider_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(seek_button_vbox), seek_slider_box,
			   FALSE, FALSE, 0);

	/* use the previous defined slider and label here */
	gtk_box_pack_start(GTK_BOX(seek_slider_box), hscale, FALSE, FALSE,
			   0);
	gtk_box_pack_start(GTK_BOX(seek_slider_box), maxsong_label, FALSE,
			   FALSE, 0);
	gtk_box_pack_start(GTK_BOX(seek_slider_box), ntsc_switch, FALSE,
			   FALSE, 0);

	if (ud_subsong_arrows == TRUE) {
	    /* use the previous defined buttons here */
	    gtk_box_pack_start_defaults(GTK_BOX(seek_button_vbox),
					next_button_frame);
	    gtk_container_add(GTK_CONTAINER(next_button_frame),
			      next_button);
	}

	if (lr_subsong_arrows == TRUE) {
	    gtk_box_pack_start_defaults(GTK_BOX(seek_button_box),
					next2_button);
	}

	gtk_widget_show_all(seekpopup);

    } else {
	gdk_window_raise(seekpopup->window);
    }
}



static void uade_seek_directly()
{
    int subsong;

    subsong = (gint) GTK_ADJUSTMENT(subsong_adj)->value;	/*get the subsong the user selected from scale */
    seek(subsong, "user request");	/* the actual subsong changing happens here ! */

    /*update current subsong in fileinfo window when it is open */
    if (fileinfowin) {
	fileinfo_update();
	//in case our seekpopup gets covered by the fileinfo :) 
	if (seekpopup)
	    gdk_window_raise(seekpopup->window);
    }
}


/* uade_seek_next() and uade_seek_previous ()
 *
 * Some desc: the functions get_next_subsong() and get_previous_previous() in
 * uade.c only do some checking and return the new valid subsongnumber.
 * the subsongchanging is done in uade_seek_directly()!
 *
 * uade_seek_directly itself is not (!) called literally from uade_seek_next()
 * and previous, but we are updating the value of hscale and thanks to 
 * GTK signalling the uade_seek_directly is invoked automatically.
 * A bit tricky but it works ;-)
 */

static void uade_seek_next()
{
    int subsong;

    subsong = get_next_subsong();	/*just returns subsong++, */


    if (subsong != -1) {
	/* Bad, bad hack to increment the upper boundary of our subsong seeker
	 * UADE really, really needs a reliable maxsubsong ! :-((((
	 */

	if ((GTK_ADJUSTMENT(subsong_adj)->upper < subsong)) {
	    fprintf(stderr, "%d\n", get_max_subsong());

	    GTK_ADJUSTMENT(subsong_adj)->upper = subsong;
	    gtk_adjustment_changed(GTK_ADJUSTMENT(subsong_adj));
	}
	/*call update scale with new subsong */
	uade_seek_update_display(subsong);
    }
}

static void uade_seek_previous()
{
    int subsong;

    subsong = get_previous_subsong();	/* just returns subsong-- */
    if (subsong != -1) {
	/*call update scale with new subsong */
	uade_seek_update_display(subsong);
    }
}

static void uade_seek_update_display(int subsong)
{
    /*update scale with new subsong value */
    GTK_ADJUSTMENT(subsong_adj)->value = subsong;
    gtk_adjustment_value_changed(GTK_ADJUSTMENT(subsong_adj));	/*here GTK gets the signal */
}


static void focus_out_event()
{
    //fprintf (stderr,"seekpopup lost its focus\n");
    gtk_widget_destroy(seekpopup);
}

static void uade_switch_ntsc()
{
    //fprintf (stderr,"uade ntsc toggle");
    set_ntsc_pal(gtk_toggle_button_get_active
		 (GTK_TOGGLE_BUTTON(ntsc_switch)));

}

/* Don't know. Is it a good thing to clean up our mess (aka windows)
 * after stop() was called ??? 
 */

void uade_close_win()
{
    if (seekpopup)
	gtk_widget_destroy(seekpopup);	/*close "< [-   ] >" popup  */
    //if (fileinfowin) gtk_widget_destroy(fileinfowin); /*close fileinfo win*/
}
