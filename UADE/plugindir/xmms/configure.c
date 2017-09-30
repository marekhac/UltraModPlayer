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
#include <ctype.h>
#include <sys/types.h>

#include "defaults.h"
#include "uade.h"
#include "gui.h"
#include "uade-os.h"
#include "../config.h"

#include "../osdep/strl.c"

/* Configuration related variables */

GtkWidget *configurewin = NULL;

GtkWidget *timeout_val_entry;
GtkWidget *silence_timeout_val_entry;
GtkWidget *use_ntsc_button;
GtkWidget *use_timeout_button;
GtkWidget *use_songend_button;

GtkWidget *force_play_button;

GtkWidget *lr_subsong_arrows_button;
GtkWidget *ud_subsong_arrows_button;
GtkWidget *use_xmms_slider_button;

GtkWidget *fileinfo_on_play_button;

GtkWidget *filemagic_check_button;
GtkWidget *filemagic_decr_button;
GtkWidget *sequential_subsongs_button;
GtkWidget *next_subsong_on_timeout_button;

GtkObject *mixing_adj;
GtkObject *volume_gain_adj;
GtkObject *lp_filter_adj;
GtkWidget *do_mixing_button;
GtkWidget *do_volume_gain_button;
GtkWidget *do_lp_filter_button;

GtkWidget *auto_db_saves_button;

int use_next_subsong_on_timeout;	/* used internally by the GUI */
int use_auto_db_saves;

/* uadeformat edit related data*/

#include <sys/types.h>
#include <dirent.h>
#include <string.h>

GtkWidget *formatswin = NULL;

GtkWidget *formats_clist;
GtkWidget *formats_player_entry;
GtkWidget *formats_ext_entry;

char curr_formatsfilename[UADE_PATH_MAX];

static void formatsfile_edit(void);

static void select_clist_row(void);
static void modify_clist_row(void);
static void add_clist_row(void);
static void delete_clist_row(void);

static void uadeformats_to_clist(void);
static int clist_to_formatsfile();
static void clist_to_formatsfile_and_quit();
static void resolve_path(char *dst, char *src, int maxlen);

#ifdef KDE2_MIMETYPE
static void formatsfile_to_kde2mime();
#endif
#ifdef ROX_MIMETYPE
static void formatsfile_to_roxmime();
#endif
#ifdef SMI_MIMETYPE
static void formatsfile_to_smimime();
#endif

static void arrows_on_off_toggle();
static void decr_on_off_toggle();

static void sequential_subsongs_on_off_toggle();
static void use_sequential_subsongs_on_off();

static void auto_db_saves_on_off_toggle();
static void use_auto_db_saves_on_off();

static void update_uadeformats_cache();


/* uade_configread & uade_configwrite
 *
 * use of the standard xmms configuration file to load/store state of the 
 * options
 */

void uade_configread(void)
{
  ConfigFile *cfg;
  cfg = xmms_cfg_open_default_file();
  xmms_cfg_read_string(cfg, "uade", "timeout_val", &timeout_val);
  xmms_cfg_read_string(cfg, "uade", "silence_timeout_val",
		       &silence_timeout_val);
  xmms_cfg_read_boolean(cfg, "uade", "use_timeout", &use_timeout);
  xmms_cfg_read_boolean(cfg, "uade", "use_songend", &use_songend);
  xmms_cfg_read_boolean(cfg, "uade", "force_by_default",
			&force_by_default);
  xmms_cfg_read_boolean(cfg, "uade", "lr_subsong_arrows",
			&lr_subsong_arrows);
  xmms_cfg_read_boolean(cfg, "uade", "ud_subsong_arrows",
			&ud_subsong_arrows);
  xmms_cfg_read_boolean(cfg, "uade", "fileinfo_on_play",
			&fileinfo_on_play);
  xmms_cfg_read_boolean(cfg, "uade", "use_ntsc", &use_ntsc);
  xmms_cfg_read_boolean(cfg, "uade", "use_xmms_slider", &use_xmms_slider);
  xmms_cfg_read_boolean(cfg, "uade", "filemagic_check", &filemagic_check);
  xmms_cfg_read_boolean(cfg, "uade", "filemagic_decr", &filemagic_decr);
  xmms_cfg_read_boolean(cfg, "uade", "sequential_subsongs",
			&sequential_subsongs);
  xmms_cfg_read_boolean(cfg, "uade", "next_subsong_on_timeout",
			&use_next_subsong_on_timeout);
  xmms_cfg_read_boolean(cfg, "uade", "do_mixing", &do_mixing);
  xmms_cfg_read_boolean(cfg, "uade", "do_volume_gain", &do_volume_gain);
  xmms_cfg_read_float(cfg, "uade", "mixing_parameter", &mixing_parameter);
  xmms_cfg_read_float(cfg, "uade", "volume_gain_parameter",
		      &volume_gain_parameter);
  xmms_cfg_read_boolean(cfg, "uade", "do_lp_filter", &do_lp_filter);
  xmms_cfg_read_int(cfg, "uade", "lp_filter", &lp_filter);

  xmms_cfg_read_boolean(cfg, "uade", "auto_db_saves", &use_auto_db_saves);
  xmms_cfg_read_int(cfg, "uade", "auto_db_save_interval", &auto_db_save_interval);

  xmms_cfg_free(cfg);

  use_sequential_subsongs_on_off();
  use_auto_db_saves_on_off();
}



/* uade_configwrite
 * Things to come are commented out for later implementation in the gui
 * however loading and saving of those values already work. The user is just
 * not able to change anything from a gui, but has to lay his/her hands on the
 * configfile in the ~/.xmms/ directory with an editor. (aka "the unix way";)
 *
 */
void uade_configwrite(void)
{

  ConfigFile *cfg;

  timeout_val = g_strdup(gtk_entry_get_text(GTK_ENTRY(timeout_val_entry)));
  silence_timeout_val =
      g_strdup(gtk_entry_get_text(GTK_ENTRY(silence_timeout_val_entry)));
  use_timeout =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(use_timeout_button));

  use_songend =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(use_songend_button));
  set_song_end_possible(use_songend);

  force_by_default =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(force_play_button));
  use_ntsc =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(use_ntsc_button));
  set_ntsc_pal(use_ntsc);

  use_xmms_slider =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
				   (use_xmms_slider_button));

  lr_subsong_arrows =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
				   (lr_subsong_arrows_button));
  ud_subsong_arrows =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
				   (ud_subsong_arrows_button));

  //fileinfo_on_play = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fileinfo_on_play_button));

  filemagic_check =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
				   (filemagic_check_button));
  filemagic_decr =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
				   (filemagic_decr_button));
  sequential_subsongs =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
				   (sequential_subsongs_button));
  use_next_subsong_on_timeout =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
				   (next_subsong_on_timeout_button));
  do_mixing =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(do_mixing_button));
  mixing_parameter = (GTK_ADJUSTMENT(mixing_adj)->value) / 100;	/*get parameter for panning */
  do_volume_gain =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
				   (do_volume_gain_button));
  volume_gain_parameter = (GTK_ADJUSTMENT(volume_gain_adj)->value) / 100.0f;	/*get parameter for volume gain */
  do_lp_filter =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(do_lp_filter_button));
  lp_filter = (GTK_ADJUSTMENT(lp_filter_adj)->value);	/*get parameter for filter */

  auto_db_saves = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(auto_db_saves_button));

  use_sequential_subsongs_on_off();
  use_auto_db_saves_on_off();

  cfg = xmms_cfg_open_default_file();
  xmms_cfg_write_boolean(cfg, "uade", "use_timeout", use_timeout);
  xmms_cfg_write_boolean(cfg, "uade", "use_songend", use_songend);
  xmms_cfg_write_string(cfg, "uade", "timeout_val", timeout_val);
  xmms_cfg_write_string(cfg, "uade", "silence_timeout_val",
			silence_timeout_val);
  xmms_cfg_write_boolean(cfg, "uade", "force_by_default",
			 force_by_default);
  xmms_cfg_write_boolean(cfg, "uade", "lr_subsong_arrows",
			 lr_subsong_arrows);
  xmms_cfg_write_boolean(cfg, "uade", "ud_subsong_arrows",
			 ud_subsong_arrows);
  xmms_cfg_write_boolean(cfg, "uade", "fileinfo_on_play",
			 fileinfo_on_play);
  xmms_cfg_write_boolean(cfg, "uade", "use_ntsc", use_ntsc);
  xmms_cfg_write_boolean(cfg, "uade", "use_xmms_slider", use_xmms_slider);
  xmms_cfg_write_boolean(cfg, "uade", "filemagic_check", filemagic_check);
  xmms_cfg_write_boolean(cfg, "uade", "filemagic_decr", filemagic_decr);
  xmms_cfg_write_boolean(cfg, "uade", "sequential_subsongs",
			 sequential_subsongs);
  xmms_cfg_write_boolean(cfg, "uade", "next_subsong_on_timeout",
			 next_subsong_on_timeout);
  xmms_cfg_write_boolean(cfg, "uade", "do_mixing", do_mixing);
  xmms_cfg_write_boolean(cfg, "uade", "do_volume_gain", do_volume_gain);
  xmms_cfg_write_float(cfg, "uade", "mixing_parameter", mixing_parameter);
  xmms_cfg_write_float(cfg, "uade", "volume_gain_parameter",
		       volume_gain_parameter);
  xmms_cfg_write_boolean(cfg, "uade", "do_lp_filter", do_lp_filter);
  xmms_cfg_write_int(cfg, "uade", "lp_filter", lp_filter);

  xmms_cfg_write_boolean(cfg, "uade", "auto_db_saves", auto_db_saves);
  xmms_cfg_write_int(cfg, "uade", "auto_db_save_interval", auto_db_save_interval);

  xmms_cfg_write_default_file(cfg);
  xmms_cfg_free(cfg);
}


static void uade_configure_ok_cb(void)
{
  uade_configwrite();		/*save the rest of the config */
  gtk_widget_destroy(configurewin);
}


/* The GTK Dialog for the uade xmms configure plugin */

void uade_configure(void)
{
  GtkWidget *configure_base_vbox;
  GtkWidget *configure_notebook;
  GtkWidget *configure_vbox;

  GtkWidget *timeout_vbox;
  GtkWidget *conf_silencetime_frame, *conf_silencetime_vbox;
  GtkWidget *conf_time_frame, *conf_time_vbox;
  GtkWidget *conf_use_ntsc_frame, *conf_use_ntsc_vbox;

  GtkWidget *compatibility_vbox;
  GtkWidget *conf_comp_frame, *conf_comp_vbox;

  GtkWidget *conf_mix_hbox, *conf_mix_frame;
  GtkWidget *conf_mix_hscale;

  GtkWidget *conf_vol_hbox, *conf_vol_frame;
  GtkWidget *conf_vol_hscale;

  GtkWidget *lp_filter_hbox, *lp_filter_frame;
  GtkWidget *lp_filter_hscale;

  GtkWidget *conf_ui_vbox;
  GtkWidget *conf_subsong_frame, *conf_subsong_vbox;
  //GtkWidget *conf_fileinfo_frame, conf_fileinfo_frame;

  GtkWidget *conf_system_frame, *conf_system_vbox, *conf_sys_vbox;

  GtkWidget *uade_formatsedit_button_box;
  GtkWidget *uade_formatsedit_button;

#ifdef KDE2_MIMETYPE
  GtkWidget *uade_createmime_button_box;
  GtkWidget *uade_createkde2mime_button;
#endif

#ifdef ROX_MIMETYPE
  GtkWidget *uade_createmime_button_box;
  GtkWidget *uade_createroxmime_button;
#endif

#ifdef SMI_MIMETYPE
  GtkWidget *uade_createmime_button_box;
  GtkWidget *uade_createmime_button;
#endif

  GtkWidget *configure_button_box;
  GtkWidget *ok_button, *cancel_button, *apply_button;

  GtkTooltips *tooltips;

  if (!configurewin)
   {

    tooltips = gtk_tooltips_new();


    configurewin = gtk_window_new(GTK_WINDOW_DIALOG);
    gtk_window_set_title(GTK_WINDOW(configurewin),
			 "UADE input plugin configuration");
    gtk_window_set_policy(GTK_WINDOW(configurewin), FALSE, FALSE, FALSE);
    gtk_window_set_position(GTK_WINDOW(configurewin), GTK_WIN_POS_MOUSE);
    gtk_container_set_border_width(GTK_CONTAINER(configurewin), 10);
    gtk_widget_set_usize(configurewin, 336, -1);
    gtk_signal_connect(GTK_OBJECT(configurewin), "destroy",
		       GTK_SIGNAL_FUNC(gtk_widget_destroyed),
		       &configurewin);

//Start of Contents Box

    configure_base_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(configurewin), configure_base_vbox);

//Start of Notebook
    configure_notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(configure_base_vbox), configure_notebook,
		       TRUE, TRUE, 0);

//Start of Contents Box for page 1

    configure_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(configure_vbox), 5);


//Start of Ntsc/Pal

    conf_use_ntsc_frame = gtk_frame_new("PAL/NTSC Timing:");
    gtk_box_pack_start(GTK_BOX(configure_vbox), conf_use_ntsc_frame, TRUE,
		       TRUE, 0);

    conf_use_ntsc_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(conf_use_ntsc_vbox), 5);
    gtk_container_add(GTK_CONTAINER(conf_use_ntsc_frame),
		      conf_use_ntsc_vbox);

    use_ntsc_button = gtk_check_button_new_with_label("use NTSC [60hz]");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(use_ntsc_button),
				 use_ntsc);
    gtk_widget_ref(use_ntsc_button);
    gtk_object_set_data_full(GTK_OBJECT(configurewin), "use_ntsc_button",
			     use_ntsc_button,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_tooltips_set_tip(tooltips, use_ntsc_button,
			 "some replayers used the vertical interrupt for timing of the playing speed of the module.\nThis led to the problems that sometimes music written for an Amiga in a country using NTSC/60Hz for the display ran too slow on pal machines and vice versa.\nUnfortunately there's no way to tell if a certain music was written on an NTSC/60hz or PAL/50hz system.\nToggle this option, which is also available on the subsong changer, if you think the music is running too slow/fast\n(default: disabled)",
			 NULL);


    gtk_box_pack_start(GTK_BOX(conf_use_ntsc_vbox), use_ntsc_button, FALSE,
		       FALSE, 0);

// end of NTSC/PAL frame.

//Start of Panning

    conf_mix_frame = gtk_frame_new("Panning (%):");
    gtk_box_pack_start(GTK_BOX(configure_vbox), conf_mix_frame, TRUE, TRUE,
		       0);

    conf_mix_hbox = gtk_hbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(conf_mix_hbox), 5);
    gtk_container_add(GTK_CONTAINER(conf_mix_frame), conf_mix_hbox);

    do_mixing_button = gtk_check_button_new_with_label("enable");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(do_mixing_button),
				 do_mixing);
    gtk_widget_ref(do_mixing_button);
    gtk_object_set_data_full(GTK_OBJECT(configurewin), "do_mixing_button",
			     do_mixing_button,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_tooltips_set_tip(tooltips, do_mixing_button,
			 "Mixes n% of the left to the right and vice versa.\nNormally the soundoutput is like on the original Amiga with an extreme stereo layout (no overlapping of the audio).If enabled uade will mix n% of the left to the right and vice versa.\nUseful if you want to listen to the music with headphones\n(default: disabled)",
			 NULL);

    mixing_adj =
	gtk_adjustment_new(mixing_parameter * 100, 0, 100, 1, 0, 0);
    conf_mix_hscale = gtk_hscale_new(GTK_ADJUSTMENT(mixing_adj));
    gtk_widget_set_usize(conf_mix_hscale, 180, -1);
    gtk_scale_set_digits(GTK_SCALE(conf_mix_hscale), 0);
    gtk_scale_set_value_pos(GTK_SCALE(conf_mix_hscale), GTK_POS_BOTTOM);
    gtk_scale_set_draw_value(GTK_SCALE(conf_mix_hscale), TRUE);
    //gtk_range_set_update_policy (GTK_RANGE (conf_mix_hscale), GTK_UPDATE_DISCONTINUOUS);



    gtk_box_pack_start(GTK_BOX(conf_mix_hbox), do_mixing_button, FALSE,
		       FALSE, 0);
    gtk_box_pack_start(GTK_BOX(conf_mix_hbox), conf_mix_hscale, FALSE,
		       FALSE, 0);
//End of Panning

//Start of Volume Gain
    conf_vol_frame =
	gtk_frame_new("Volume Gain (%): 100% is normal (no effect)");
    gtk_box_pack_start(GTK_BOX(configure_vbox), conf_vol_frame, TRUE, TRUE,
		       0);
    conf_vol_hbox = gtk_hbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(conf_vol_hbox), 5);
    gtk_container_add(GTK_CONTAINER(conf_vol_frame), conf_vol_hbox);


    do_volume_gain_button = gtk_check_button_new_with_label("enable");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(do_volume_gain_button),
				 do_volume_gain);
    gtk_widget_ref(do_volume_gain_button);
    gtk_object_set_data_full(GTK_OBJECT(configurewin),
			     "do_volume_gain_button",
			     do_volume_gain_button,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_tooltips_set_tip(tooltips, do_volume_gain_button,
			 "Useful if you want to set the volume of the amiga music to a simlar level as mp3, ogg etc.\n(default: disabled)",
			 NULL);


    volume_gain_adj =
	gtk_adjustment_new(volume_gain_parameter * 100.0f, 0, 200, 1, 0,
			   0);
    conf_vol_hscale = gtk_hscale_new(GTK_ADJUSTMENT(volume_gain_adj));
    gtk_widget_set_usize(conf_vol_hscale, 180, -1);
    gtk_scale_set_digits(GTK_SCALE(conf_vol_hscale), 0);
    gtk_scale_set_value_pos(GTK_SCALE(conf_vol_hscale), GTK_POS_BOTTOM);
    gtk_scale_set_draw_value(GTK_SCALE(conf_vol_hscale), TRUE);
    //gtk_range_set_update_policy (GTK_RANGE (conf_vol_hscale), GTK_UPDATE_DISCONTINUOUS);
    gtk_box_pack_start(GTK_BOX(conf_vol_hbox), do_volume_gain_button,
		       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(conf_vol_hbox), conf_vol_hscale, FALSE,
		       FALSE, 0);
//End of Volume Gain

    lp_filter_frame = gtk_frame_new("Lowpass filter (1-4):");
    gtk_box_pack_start(GTK_BOX(configure_vbox), lp_filter_frame, TRUE,
		       TRUE, 0);
    lp_filter_hbox = gtk_hbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(lp_filter_hbox), 5);
    gtk_container_add(GTK_CONTAINER(lp_filter_frame), lp_filter_hbox);

    do_lp_filter_button = gtk_check_button_new_with_label("enable");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(do_lp_filter_button),
				 do_lp_filter);
    gtk_widget_ref(do_lp_filter_button);
    gtk_object_set_data_full(GTK_OBJECT(configurewin),
			     "do_lp_filter_button", do_lp_filter_button,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_tooltips_set_tip(tooltips, do_lp_filter_button,
			 "Enable/disable lowpass filtering (default: disabled)",
			 NULL);

    lp_filter_adj = gtk_adjustment_new(lp_filter, 1, 4, 1, 0, 0);
    lp_filter_hscale = gtk_hscale_new(GTK_ADJUSTMENT(lp_filter_adj));
    gtk_widget_set_usize(lp_filter_hscale, 180, -1);
    gtk_scale_set_digits(GTK_SCALE(lp_filter_hscale), 0);
    gtk_scale_set_value_pos(GTK_SCALE(lp_filter_hscale), GTK_POS_BOTTOM);
    gtk_scale_set_draw_value(GTK_SCALE(lp_filter_hscale), TRUE);
    gtk_box_pack_start(GTK_BOX(lp_filter_hbox), do_lp_filter_button, FALSE,
		       FALSE, 0);
    gtk_box_pack_start(GTK_BOX(lp_filter_hbox), lp_filter_hscale, FALSE,
		       FALSE, 0);

// end of contents box of page 1

    gtk_notebook_append_page(GTK_NOTEBOOK(configure_notebook),
			     configure_vbox, gtk_label_new("Options"));

//Start of page 2

    timeout_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(timeout_vbox), 5);

    conf_time_frame = gtk_frame_new("Timeout: (min:sec)");
    gtk_box_pack_start(GTK_BOX(timeout_vbox), conf_time_frame, TRUE, TRUE,
		       0);

    conf_time_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(conf_time_vbox), 5);
    gtk_container_add(GTK_CONTAINER(conf_time_frame), conf_time_vbox);

    timeout_val_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(timeout_val_entry), timeout_val);

    gtk_box_pack_start(GTK_BOX(conf_time_vbox), timeout_val_entry, FALSE,
		       FALSE, 0);

    use_timeout_button =
	gtk_check_button_new_with_label("timeout for looping modules");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(use_timeout_button),
				 use_timeout);
    gtk_signal_connect(GTK_OBJECT(use_timeout_button), "clicked",
		       GTK_SIGNAL_FUNC(sequential_subsongs_on_off_toggle),
		       NULL);
    gtk_widget_ref(use_timeout_button);
    gtk_object_set_data_full(GTK_OBJECT(configurewin),
			     "use_timeout_button", use_timeout_button,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_tooltips_set_tip(tooltips, use_timeout_button,
			 "Some amiga music formats would play infinitely, so uade does not really know how long a file plays. To end such 'endless modules' set the timeout to a certain amount of time to force an end to these (sub)songs and skip to the next subsong/song in the playlist\n(default timeout: 5:00 min, enabled)",
			 NULL);

    use_songend_button =
	gtk_check_button_new_with_label("songend enabled");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(use_songend_button),
				 use_songend);
    gtk_signal_connect(GTK_OBJECT(use_songend_button), "clicked",
		       GTK_SIGNAL_FUNC(sequential_subsongs_on_off_toggle),
		       NULL);
    gtk_widget_ref(use_songend_button);
    gtk_object_set_data_full(GTK_OBJECT(configurewin),
			     "use_songend_button", use_songend_button,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_tooltips_set_tip(tooltips, use_songend_button,
			 "some replayers message when a song is over instead of playing it infinetely\nif enabled uade will skip to the next subsong and/or song in the xmms playlist.\n(default: enabled)",
			 NULL);

    next_subsong_on_timeout_button =
	gtk_check_button_new_with_label
	("switch to next subsong on timeout");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
				 (next_subsong_on_timeout_button),
				 use_next_subsong_on_timeout);
    gtk_widget_ref(next_subsong_on_timeout_button);
    gtk_object_set_data_full(GTK_OBJECT(configurewin),
			     "next_subsong_on_timeout_button",
			     next_subsong_on_timeout_button,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_tooltips_set_tip(tooltips, next_subsong_on_timeout_button,
			 "uses the timeout for looping subsongs, instead of the whole file... ",
			 NULL);

    sequential_subsongs_button =
	gtk_check_button_new_with_label
	("switch to next subsong on songend");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
				 (sequential_subsongs_button),
				 sequential_subsongs);
    sequential_subsongs_on_off_toggle();
    gtk_widget_ref(sequential_subsongs_button);
    gtk_object_set_data_full(GTK_OBJECT(configurewin),
			     "sequential_subsongs_button",
			     sequential_subsongs_button,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_tooltips_set_tip(tooltips, sequential_subsongs_button,
			 "(formerly known as 'play subsongs in sequence')\nUnlike mp3 or oggs, many Amiga music formats mostly used in games, have socalled subsongs (even some *.MOD files have pseudo subsongs, btw.) They were used for jingles, level music, etc.\nTo listen to these subsongs inside an amiga music format in a row you need to enable this option.(default: enabled)",
			 NULL);

    gtk_box_pack_start(GTK_BOX(conf_time_vbox), use_timeout_button, FALSE,
		       FALSE, 0);
    gtk_box_pack_start(GTK_BOX(conf_time_vbox), use_songend_button, FALSE,
		       FALSE, 0);
    gtk_box_pack_start(GTK_BOX(conf_time_vbox),
		       next_subsong_on_timeout_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(conf_time_vbox), sequential_subsongs_button,
		       FALSE, FALSE, 0);

    conf_silencetime_frame = gtk_frame_new("Timeout after silence: (sec)");
    gtk_box_pack_start(GTK_BOX(timeout_vbox), conf_silencetime_frame, TRUE,
		       TRUE, 0);

    conf_silencetime_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(conf_silencetime_vbox),
				   5);
    gtk_container_add(GTK_CONTAINER(conf_silencetime_frame),
		      conf_silencetime_vbox);

    silence_timeout_val_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(silence_timeout_val_entry),
		       silence_timeout_val);

    gtk_box_pack_start(GTK_BOX(conf_silencetime_vbox),
		       silence_timeout_val_entry, FALSE, FALSE, 0);


// end of timeout frame.


// end of contents box of page 2

    gtk_notebook_append_page(GTK_NOTEBOOK(configure_notebook),
			     timeout_vbox, gtk_label_new("Songend"));


//Start of Contents Box for page 3

    compatibility_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(compatibility_vbox), 5);

//Start of Compatibility frame, text and option widgets

    conf_comp_frame = gtk_frame_new("Compatibility:");
    gtk_box_pack_start(GTK_BOX(compatibility_vbox), conf_comp_frame, TRUE,
		       TRUE, 0);

    conf_comp_vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(conf_comp_vbox), 5);
    gtk_container_add(GTK_CONTAINER(conf_comp_frame), conf_comp_vbox);

    filemagic_check_button =
	gtk_check_button_new_with_label
	("detect some files by contents,\ninstead of using the prefix/suffix.");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(filemagic_check_button),
				 filemagic_check);
    gtk_widget_ref(filemagic_check_button);
    gtk_object_set_data_full(GTK_OBJECT(configurewin),
			     "filemagic_check_button",
			     filemagic_check_button,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_tooltips_set_tip(tooltips, filemagic_check_button,
			 "Normally uade relies on the right prefix/extension of a music file to choose the right replayer.\nTo play  formats like MOD/ADSC, TFMX, TFMX_Pro, TFMX_7V, different formats which have the same file extension/prefix, or for renamed files with a unkown or wrong file extension, you have to enable this option. Maybe uade is able to detect the right format anyway. Refer for the docs for further help.\nBeware: It might slow down things though.\n(default: disabled)",
			 NULL);

    filemagic_decr_button =
	gtk_check_button_new_with_label
	("decrunch packed files before filecheck");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(filemagic_decr_button),
				 filemagic_decr);
    gtk_widget_ref(filemagic_decr_button);
    gtk_object_set_data_full(GTK_OBJECT(configurewin),
			     "filemagic_decr_button",
			     filemagic_decr_button,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_tooltips_set_tip(tooltips, filemagic_decr_button,
			 "useful if you also want to use the detection by contents with packed files like *.gz, *.zip, etc.\nWarning: it slows down adding files to the xmms playlist, which can be very annyoing if you try to load files over a network\n(default: disabled)",
			 NULL);


    gtk_signal_connect(GTK_OBJECT(filemagic_check_button), "clicked",
		       GTK_SIGNAL_FUNC(decr_on_off_toggle), NULL);
    decr_on_off_toggle();

    force_play_button =
	gtk_check_button_new_with_label
	("Force players to play tunes\n(disable player's module recognition)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(force_play_button),
				 force_by_default);
    gtk_widget_ref(force_play_button);
    gtk_object_set_data_full(GTK_OBJECT(configurewin), "force_play_button",
			     force_play_button,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_tooltips_set_tip(tooltips, force_play_button,
			 "Compatibility hack for 'bitchy' replayers.\nUseful if a replayer refuses to play a certain song to 'convince' it to play it anyway.\nChances are the replayer running in the emulator crashes, but it should be fairly save to use this option if you need it.\n(default: enabled)",
			 NULL);


    gtk_box_pack_start(GTK_BOX(conf_comp_vbox), filemagic_check_button,
		       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(conf_comp_vbox), filemagic_decr_button,
		       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(conf_comp_vbox), force_play_button, FALSE,
		       FALSE, 0);

// end of Compatibility frame.

// end of contents box of page 3

    gtk_notebook_append_page(GTK_NOTEBOOK(configure_notebook),
			     compatibility_vbox,
			     gtk_label_new("Compatibility"));
// End of page 1

//Start of Contents Box for page 4

    conf_ui_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(conf_ui_vbox), 5);

/* Subsongseeker frame */
    conf_subsong_frame = gtk_frame_new("configure subsong changer:");
    gtk_box_pack_start(GTK_BOX(conf_ui_vbox), conf_subsong_frame, TRUE,
		       TRUE, 0);

    conf_subsong_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(conf_subsong_vbox), 5);
    gtk_container_add(GTK_CONTAINER(conf_subsong_frame),
		      conf_subsong_vbox);

    lr_subsong_arrows_button =
	gtk_check_button_new_with_label("arrows on left and right");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
				 (lr_subsong_arrows_button),
				 lr_subsong_arrows);

    ud_subsong_arrows_button =
	gtk_check_button_new_with_label("arrows on top and bottom");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
				 (ud_subsong_arrows_button),
				 ud_subsong_arrows);

    use_xmms_slider_button =
	gtk_check_button_new_with_label
	("\ndisable uade's subsong changer and use \nxmms` slider bar to change subsongs.");
    gtk_signal_connect(GTK_OBJECT(use_xmms_slider_button), "clicked",
		       GTK_SIGNAL_FUNC(arrows_on_off_toggle), NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(use_xmms_slider_button),
				 use_xmms_slider);
    arrows_on_off_toggle();

    gtk_box_pack_start(GTK_BOX(conf_subsong_vbox),
		       lr_subsong_arrows_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(conf_subsong_vbox),
		       ud_subsong_arrows_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(conf_ui_vbox), use_xmms_slider_button,
		       FALSE, FALSE, 0);

/* End of Subsongseeker frame */

/* Fileinfo config frame */

/*  conf_fileinfo_frame = gtk_frame_new ("Fileinfo:");
  gtk_box_pack_start (GTK_BOX (conf_ui_vbox), conf_fileinfo_frame, TRUE, TRUE, 0); 

  conf_fileinfo_vbox = gtk_vbox_new(FALSE, 10);
  gtk_container_set_border_width(GTK_CONTAINER(conf_fileinfo_vbox), 5);
  gtk_container_add(GTK_CONTAINER(conf_fileinfo_frame), conf_fileinfo_vbox);

  fileinfo_on_play_button = gtk_check_button_new_with_label("\nopen uade's fileinfo on play\n(Sorry, not working, yet)");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fileinfo_on_play_button), fileinfo_on_play);

  gtk_box_pack_start(GTK_BOX(conf_fileinfo_vbox), fileinfo_on_play_button, FALSE, FALSE, 0);
*/

// end of contents box of page 4

    gtk_notebook_append_page(GTK_NOTEBOOK(configure_notebook),
			     conf_ui_vbox, gtk_label_new("GUI"));

// End of page 4


//Start of Contents Box for page 5

    conf_system_vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(conf_system_vbox), 5);

//Start of Paths frame

    conf_system_frame = gtk_frame_new("System");
    gtk_box_pack_start(GTK_BOX(conf_system_vbox), conf_system_frame, TRUE,
		       TRUE, 0);

    conf_sys_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(conf_sys_vbox), 5);
    gtk_container_add(GTK_CONTAINER(conf_system_frame), conf_sys_vbox);

#ifdef KDE2_MIMETYPE
    uade_createmime_button_box = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(uade_createmime_button_box),
			      GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(uade_createmime_button_box),
			       5);
    gtk_box_pack_start(GTK_BOX(conf_system_vbox),
		       uade_createmime_button_box, FALSE, FALSE, 0);


    uade_createkde2mime_button =
	gtk_button_new_with_label("Create KDE2 Filetype");
    GTK_WIDGET_SET_FLAGS(uade_createkde2mime_button, GTK_CAN_DEFAULT);
    gtk_signal_connect(GTK_OBJECT(uade_createkde2mime_button), "clicked",
		       GTK_SIGNAL_FUNC(formatsfile_to_kde2mime), NULL);

    gtk_box_pack_start_defaults(GTK_BOX(uade_createmime_button_box),
				uade_createkde2mime_button);
#endif

#ifdef ROX_MIMETYPE
    uade_createmime_button_box = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(uade_createmime_button_box),
			      GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(uade_createmime_button_box),
			       5);
    gtk_box_pack_start(GTK_BOX(conf_system_vbox),
		       uade_createmime_button_box, FALSE, FALSE, 0);


    uade_createroxmime_button =
	gtk_button_new_with_label("Create ROX 1.2x MIMEtype");
    GTK_WIDGET_SET_FLAGS(uade_createroxmime_button, GTK_CAN_DEFAULT);
    gtk_signal_connect(GTK_OBJECT(uade_createroxmime_button), "clicked",
		       GTK_SIGNAL_FUNC(formatsfile_to_roxmime), NULL);

    gtk_box_pack_start_defaults(GTK_BOX(uade_createmime_button_box),
				uade_createroxmime_button);
#endif

#ifdef SMI_MIMETYPE
    uade_createmime_button_box = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(uade_createmime_button_box),
			      GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(uade_createmime_button_box),
			       5);
    gtk_box_pack_start(GTK_BOX(conf_system_vbox),
		       uade_createmime_button_box, FALSE, FALSE, 0);


    uade_createmime_button =
	gtk_button_new_with_label("Update shared mime info database");
    GTK_WIDGET_SET_FLAGS(uade_createmime_button, GTK_CAN_DEFAULT);
    gtk_signal_connect(GTK_OBJECT(uade_createmime_button), "clicked",
		       GTK_SIGNAL_FUNC(formatsfile_to_smimime), NULL);

    gtk_box_pack_start_defaults(GTK_BOX(uade_createmime_button_box),
				uade_createmime_button);
#endif

    /* auto db saves button */
    auto_db_saves_button =
      gtk_check_button_new_with_label("auto db saves");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(auto_db_saves_button),
				 use_auto_db_saves);
    gtk_signal_connect(GTK_OBJECT(auto_db_saves_button), "clicked",
		       GTK_SIGNAL_FUNC(auto_db_saves_on_off_toggle),
		       NULL);
    gtk_widget_ref(auto_db_saves_button);
    gtk_object_set_data_full(GTK_OBJECT(conf_system_vbox),
			     "auto db saves", auto_db_saves_button,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_tooltips_set_tip(tooltips, auto_db_saves_button,
			 "Toggle song duration database automatic saving ON / OFF. If this option is ON, the database is saved every now and then.",
			 NULL);
    gtk_box_pack_start(GTK_BOX(conf_sys_vbox), auto_db_saves_button, FALSE,
		       FALSE, 0);

    /* format editor button */
    uade_formatsedit_button_box = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(uade_formatsedit_button_box),
			      GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(uade_formatsedit_button_box),
			       5);
    gtk_box_pack_start(GTK_BOX(conf_system_vbox),
		       uade_formatsedit_button_box, FALSE, FALSE, 0);

    uade_formatsedit_button =
	gtk_button_new_with_label("Edit uadeformats file...");
    GTK_WIDGET_SET_FLAGS(uade_formatsedit_button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default(uade_formatsedit_button);
    gtk_signal_connect(GTK_OBJECT(uade_formatsedit_button), "clicked",
		       GTK_SIGNAL_FUNC(formatsfile_edit), NULL);
    gtk_widget_ref(uade_formatsedit_button);
    gtk_object_set_data_full(GTK_OBJECT(configurewin),
			     "uade_formatsedit_button",
			     uade_formatsedit_button,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_tooltips_set_tip(tooltips, uade_formatsedit_button,
			 "The uadeformats editor\n The uadeformats file contains the list, which associates a certain prefix/extension of a file to the responsible replayer\n E.g. by associating 'AON' to 'ArtOfNoise' uade will play all AON.* or *.AON or files with the replayer called ArtOfNoise.\nYou can use this simple editor to add or change associations to 'teach' uade new formats/replayers\nhave fun",
			 NULL);


    gtk_box_pack_start_defaults(GTK_BOX(uade_formatsedit_button_box),
				uade_formatsedit_button);

// end of paths frame.


// end of contents box of page 5

    gtk_notebook_append_page(GTK_NOTEBOOK(configure_notebook),
			     conf_system_vbox, gtk_label_new("System"));

// End of page 5



// Start of Ok and Cancel Button Box

    configure_button_box = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(configure_button_box),
			      GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(configure_button_box), 5);
    gtk_box_pack_start(GTK_BOX(configure_base_vbox), configure_button_box,
		       FALSE, FALSE, 0);


    apply_button = gtk_button_new_with_label("Apply");
    GTK_WIDGET_SET_FLAGS(apply_button, GTK_CAN_DEFAULT);
    gtk_signal_connect_object(GTK_OBJECT(apply_button), "clicked",
			      GTK_SIGNAL_FUNC(uade_configwrite), NULL);


    cancel_button = gtk_button_new_with_label("Cancel");
    GTK_WIDGET_SET_FLAGS(cancel_button, GTK_CAN_DEFAULT);
    gtk_signal_connect_object(GTK_OBJECT(cancel_button), "clicked",
			      GTK_SIGNAL_FUNC(gtk_widget_destroy),
			      GTK_OBJECT(configurewin));

    ok_button = gtk_button_new_with_label("Ok");
    GTK_WIDGET_SET_FLAGS(ok_button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default(ok_button);

    gtk_signal_connect(GTK_OBJECT(ok_button), "clicked",
		       GTK_SIGNAL_FUNC(uade_configure_ok_cb), NULL);

    gtk_box_pack_start_defaults(GTK_BOX(configure_button_box), ok_button);
    gtk_box_pack_start_defaults(GTK_BOX(configure_button_box),
				cancel_button);
    gtk_box_pack_start_defaults(GTK_BOX(configure_button_box),
				apply_button);

// End of Ok and Cancel Button Box

// Activate Tooltips
    gtk_object_set_data(GTK_OBJECT(configurewin), "tooltips", tooltips);


    gtk_widget_show_all(configurewin);

  } else {
    gdk_window_raise(configurewin->window);
  }
}


static void arrows_on_off_toggle()
{
  if (gtk_toggle_button_get_active
      (GTK_TOGGLE_BUTTON(use_xmms_slider_button))) {
    gtk_widget_set_sensitive(ud_subsong_arrows_button, FALSE);
    gtk_widget_set_sensitive(lr_subsong_arrows_button, FALSE);
  } else {
    gtk_widget_set_sensitive(GTK_WIDGET(ud_subsong_arrows_button), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(lr_subsong_arrows_button), TRUE);
  }
}
static void decr_on_off_toggle()
{
  if (gtk_toggle_button_get_active
      (GTK_TOGGLE_BUTTON(filemagic_check_button))) {
    gtk_widget_set_sensitive(filemagic_decr_button, TRUE);
  } else {
    gtk_widget_set_sensitive(GTK_WIDGET(filemagic_decr_button), FALSE);
  }
}

static void sequential_subsongs_on_off_toggle()
{

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(use_timeout_button))
      ||
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(use_songend_button)))
  {
    gtk_widget_set_sensitive(sequential_subsongs_button, TRUE);
    gtk_widget_set_sensitive(next_subsong_on_timeout_button, TRUE);
  } else {
    gtk_widget_set_sensitive(GTK_WIDGET(sequential_subsongs_button),
			     FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(next_subsong_on_timeout_button),
			     FALSE);
  }


}
static void use_sequential_subsongs_on_off()
{
  if (use_timeout || use_songend) {
    next_subsong_on_song_end = sequential_subsongs;
    next_subsong_on_timeout = use_next_subsong_on_timeout;
  } else {
    next_subsong_on_song_end = FALSE;
    next_subsong_on_timeout = FALSE;
  }
}

static void auto_db_saves_on_off_toggle(void)
{
  use_auto_db_saves = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(auto_db_saves_button));
}

static void use_auto_db_saves_on_off(void)
{
  auto_db_saves = use_auto_db_saves;
}



/* UADE Formats Editor
 * (idea based on the mimetype viewers editor of nscache-0.3) 
 */

void formatsfile_edit(void)
{

  GtkWidget *formats_base_vbox;
  GtkWidget *formats_frame, *formats_vbox;
  GtkWidget *formats_swindow;
  GtkWidget *formats_ext_clisttxt, *formats_player_clisttxt;

  GtkWidget *formats_entry_hbox;
  GtkWidget *formats_player_combo;
  GList *combo_items = NULL;

  GtkWidget *formats_hbuttonbox;
  GtkWidget *formats_add_button;
  GtkWidget *formats_modify_button;
  GtkWidget *formats_delete_button;

  GtkWidget *formatswin_button_box;
  GtkWidget *ok_button, *cancel_button, *apply_button;

  DIR *dir;
  char playerdir[UADE_PATH_MAX];
  struct dirent *dent;

  if (!formatswin) {

    formatswin = gtk_window_new(GTK_WINDOW_DIALOG);
    gtk_window_set_title(GTK_WINDOW(formatswin), "UADE Formats Editor");
    gtk_window_set_policy(GTK_WINDOW(formatswin), FALSE, FALSE, FALSE);
    gtk_window_set_position(GTK_WINDOW(formatswin), GTK_WIN_POS_MOUSE);
    gtk_container_set_border_width(GTK_CONTAINER(formatswin), 10);
    gtk_signal_connect(GTK_OBJECT(formatswin), "destroy",
		       GTK_SIGNAL_FUNC(gtk_widget_destroyed), &formatswin);

//Start of Contents Box

    formats_base_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(formats_base_vbox), 5);
    gtk_container_add(GTK_CONTAINER(formatswin), formats_base_vbox);


//Start of Formats Frame

    formats_frame = gtk_frame_new("UADE Formats: ");
    gtk_box_pack_start(GTK_BOX(formats_base_vbox), formats_frame, TRUE,
		       TRUE, 0);

    formats_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(formats_vbox), 5);
    gtk_container_add(GTK_CONTAINER(formats_frame), formats_vbox);


    formats_swindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_ref(formats_swindow);
    gtk_box_pack_start(GTK_BOX(formats_vbox), formats_swindow, TRUE, TRUE,
		       0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(formats_swindow),
				   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    formats_clist = gtk_clist_new(2);
    gtk_clist_set_selection_mode(GTK_CLIST(formats_clist),
				 GTK_SELECTION_SINGLE);
    gtk_container_add(GTK_CONTAINER(formats_swindow), formats_clist);

    gtk_widget_set_usize(formats_clist, -1, 192);
    gtk_clist_set_column_width(GTK_CLIST(formats_clist), 0, 80);
    gtk_clist_set_column_width(GTK_CLIST(formats_clist), 1, 80);
    gtk_clist_column_titles_show(GTK_CLIST(formats_clist));

    formats_ext_clisttxt = gtk_label_new("Pref./Suff.");
    gtk_clist_set_column_widget(GTK_CLIST(formats_clist), 0,
				formats_ext_clisttxt);

    formats_player_clisttxt = gtk_label_new("Player");
    gtk_clist_set_column_widget(GTK_CLIST(formats_clist), 1,
				formats_player_clisttxt);
    gtk_clist_column_titles_passive(GTK_CLIST(formats_clist));


    gtk_clist_clear(GTK_CLIST(formats_clist));
    gtk_clist_freeze(GTK_CLIST(formats_clist));

    //setup list

    uadeformats_to_clist();
    gtk_clist_sort(GTK_CLIST(formats_clist));

    gtk_clist_thaw(GTK_CLIST(formats_clist));

    gtk_signal_connect(GTK_OBJECT(formats_clist), "select_row",
		       GTK_SIGNAL_FUNC(select_clist_row), NULL);


//start of format entry hbox
    formats_entry_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(formats_vbox), formats_entry_hbox, FALSE,
		       FALSE, 0);


    formats_ext_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(formats_entry_hbox), formats_ext_entry,
		       FALSE, TRUE, 0);
    gtk_widget_set_usize(formats_ext_entry, 83, -2);


    formats_player_combo = gtk_combo_new();
    gtk_box_pack_start(GTK_BOX(formats_entry_hbox), formats_player_combo,
		       TRUE, TRUE, 0);


    /* read the player_dir and build a list for the combo box. */
    if (!uade_get_path(playerdir, UADE_PATH_PLAYERDIR, sizeof(playerdir))) {
      fprintf(stderr, "uade: configure.c: could not get playerdir\n");
      playerdir[0] = 0;
    }
    dir = opendir(playerdir);
    if (!dir) {
      combo_items = g_list_append(combo_items, g_strdup_printf(" "));
    } else {
      while ((dent = readdir(dir))) {
	if (dent->d_name) {
	  if (strcmp(dent->d_name, ".") && strcmp(dent->d_name, "..")
	      && strcmp(dent->d_name, "uadeformats")
	      && strcmp(dent->d_name, "ENV:EaglePlayer")) {
	    combo_items =
		g_list_insert_sorted(combo_items,
				     g_strdup_printf("%s", dent->d_name),
				     (GCompareFunc) strcasecmp);
	  }
	}
      }
      /* add custom player to combobox */
      combo_items =
	  g_list_insert_sorted(combo_items, g_strdup_printf("custom"),
			       (GCompareFunc) strcasecmp);
    }
    /*when all went right we have our list now */


    gtk_combo_set_popdown_strings(GTK_COMBO(formats_player_combo),
				  combo_items);
    g_list_free(combo_items);

    formats_player_entry = GTK_COMBO(formats_player_combo)->entry;
    gtk_entry_set_text(GTK_ENTRY(formats_player_entry), "");


//end of format entry hbox

    formats_hbuttonbox = gtk_hbutton_box_new();
    gtk_box_pack_start(GTK_BOX(formats_vbox), formats_hbuttonbox, TRUE,
		       TRUE, 0);

    formats_add_button = gtk_button_new_with_label("add");
    gtk_container_add(GTK_CONTAINER(formats_hbuttonbox),
		      formats_add_button);
    GTK_WIDGET_SET_FLAGS(formats_add_button, GTK_CAN_DEFAULT);
    gtk_signal_connect(GTK_OBJECT(formats_add_button), "clicked",
		       GTK_SIGNAL_FUNC(add_clist_row), NULL);

    formats_modify_button = gtk_button_new_with_label("modify");
    gtk_container_add(GTK_CONTAINER(formats_hbuttonbox),
		      formats_modify_button);
    GTK_WIDGET_SET_FLAGS(formats_modify_button, GTK_CAN_DEFAULT);
    gtk_signal_connect(GTK_OBJECT(formats_modify_button), "clicked",
		       GTK_SIGNAL_FUNC(modify_clist_row), NULL);

    formats_delete_button = gtk_button_new_with_label("remove");
    gtk_container_add(GTK_CONTAINER(formats_hbuttonbox),
		      formats_delete_button);
    GTK_WIDGET_SET_FLAGS(formats_delete_button, GTK_CAN_DEFAULT);
    gtk_signal_connect(GTK_OBJECT(formats_delete_button), "clicked",
		       GTK_SIGNAL_FUNC(delete_clist_row), NULL);

// Start of Ok and Cancel Button Box

    formatswin_button_box = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(formatswin_button_box),
			      GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(formatswin_button_box), 5);
    gtk_box_pack_start(GTK_BOX(formats_base_vbox), formatswin_button_box,
		       FALSE, FALSE, 0);

    apply_button = gtk_button_new_with_label("Apply");
    GTK_WIDGET_SET_FLAGS(apply_button, GTK_CAN_DEFAULT);
    gtk_signal_connect_object(GTK_OBJECT(apply_button), "clicked",
			      GTK_SIGNAL_FUNC(clist_to_formatsfile), NULL);

    cancel_button = gtk_button_new_with_label("Cancel");
    GTK_WIDGET_SET_FLAGS(cancel_button, GTK_CAN_DEFAULT);
    gtk_signal_connect_object(GTK_OBJECT(cancel_button), "clicked",
			      GTK_SIGNAL_FUNC(gtk_widget_destroy),
			      GTK_OBJECT(formatswin));

    ok_button = gtk_button_new_with_label("Ok");
    GTK_WIDGET_SET_FLAGS(ok_button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default(ok_button);
    gtk_signal_connect_object(GTK_OBJECT(ok_button), "clicked",
			      GTK_SIGNAL_FUNC
			      (clist_to_formatsfile_and_quit), NULL);


    gtk_box_pack_start_defaults(GTK_BOX(formatswin_button_box), ok_button);
    gtk_box_pack_start_defaults(GTK_BOX(formatswin_button_box),
				cancel_button);
    gtk_box_pack_start_defaults(GTK_BOX(formatswin_button_box),
				apply_button);

// End of Ok and Cancel Button Box

    gtk_widget_show_all(formatswin);

  }

  else {
    gdk_window_raise(formatswin->window);
  }


}

/* End of formats editor window*/



static void add_clist_row()
{
  char *p[3];
  p[0] = gtk_entry_get_text(GTK_ENTRY(formats_ext_entry));
  p[1] = gtk_entry_get_text(GTK_ENTRY(formats_player_entry));

  if (!*p[0])
    p[0] = NULL;
  if (!*p[1])
    p[1] = NULL;

  if (p[0] && p[1]) {
    gtk_clist_freeze(GTK_CLIST(formats_clist));	/* better safe than sorry ? */

    gtk_clist_append(GTK_CLIST(formats_clist), p);
    gtk_clist_sort(GTK_CLIST(formats_clist));

    gtk_clist_thaw(GTK_CLIST(formats_clist));

    gtk_entry_set_text(GTK_ENTRY(formats_ext_entry), "");	/*clear entries */
    gtk_entry_set_text(GTK_ENTRY(formats_player_entry), "");


  } else {
    gdk_beep();
  }
}

static void modify_clist_row()
{
  char *p[2];
  p[0] = gtk_entry_get_text(GTK_ENTRY(formats_ext_entry));
  p[1] = gtk_entry_get_text(GTK_ENTRY(formats_player_entry));

  if (!*p[0])
    p[0] = NULL;
  if (!*p[1])
    p[1] = NULL;

  if (p[0] && p[1]) {
    guint row;
    //fprintf (stderr,"modifylist: %d\n",GTK_CLIST(formats_clist)->selection);

    if (GTK_CLIST(formats_clist)->selection != 0) {
      gtk_clist_freeze(GTK_CLIST(formats_clist));	/* better safe than sorry (neccessary?) */

      row = GPOINTER_TO_INT(GTK_CLIST(formats_clist)->selection->data);

      gtk_clist_set_text(GTK_CLIST(formats_clist), row, 0, p[0]);
      gtk_clist_set_text(GTK_CLIST(formats_clist), row, 1, p[1]);


      gtk_clist_sort(GTK_CLIST(formats_clist));
      gtk_clist_thaw(GTK_CLIST(formats_clist));



    }
  } else {
    gdk_beep();
  }
}

static void delete_clist_row()
{
  gtk_clist_freeze(GTK_CLIST(formats_clist));

  while (GTK_CLIST(formats_clist)->selection) {
    gtk_clist_remove(GTK_CLIST(formats_clist),
		     GPOINTER_TO_INT(GTK_CLIST(formats_clist)->selection->
				     data));
  }
  gtk_clist_thaw(GTK_CLIST(formats_clist));

  gtk_entry_set_text(GTK_ENTRY(formats_ext_entry), "");	/*clear entries */
  gtk_entry_set_text(GTK_ENTRY(formats_player_entry), "");

}


static void select_clist_row()
{
  gchar *p;
  p = NULL;
  gtk_clist_get_text(GTK_CLIST(formats_clist),
		     GPOINTER_TO_INT(GTK_CLIST(formats_clist)->selection->
				     data), 0, &p);
  gtk_entry_set_text(GTK_ENTRY(formats_ext_entry), p);

  p = NULL;
  gtk_clist_get_text(GTK_CLIST(formats_clist),
		     GPOINTER_TO_INT(GTK_CLIST(formats_clist)->selection->
				     data), 1, &p);
  gtk_entry_set_text(GTK_ENTRY(formats_player_entry), p);
}


static void update_uadeformats_cache()
{
  /* tell uade.c to update the uadeformats cache */

  //fprintf (stderr, "update formats cache called");

  uadeformats_is_cached = 0;

}


void uade_alert(gchar * alertext)
{
  GtkWidget *alertwin = NULL;

  alertwin = xmms_show_message("Problem while starting UADE-plugin...",
			       g_strdup_printf("%s", alertext),
			       "Close", FALSE, NULL, NULL);
  gtk_signal_connect(GTK_OBJECT(alertwin), "destroy",
		     GTK_SIGNAL_FUNC(gtk_widget_destroyed), &alertwin);
}

/* End of configure GTK Code
 * corrections and enhancements very welcome.
 * -- Mieke
 */



/*Reading and writing uadeformats */

/* uadeformats_to_clist
 * 
 * This lousy piece of code is a very dumb parsing of the uadeformats file.
 * Feel free to enhance and correct it.
 *
 * it *expects* a valid formatline like this: pref/ext[tab]playername.
 * if there isn't a [tab] the line is ignored.
 * a space is always belonging to either pref/ext or playername. 
 *
 *
 */
static void uadeformats_to_clist(void)
{
  FILE *formatsfile;
  char filename[UADE_PATH_MAX];
  char *line;
  char *formatsline_r;
  char *formatsline_l;
  gboolean formats_ok = FALSE;

  char *p[2];

  if (!uade_get_path(filename, UADE_PATH_FORMATSFILE, sizeof(filename))) {
    fprintf(stderr, "uade: uadeformats_to_clist: could not get formatsfile\n");
    return;
  }
  strlcpy(curr_formatsfilename, filename, sizeof(curr_formatsfilename));

  formatsfile = fopen(filename, "r");
  if (formatsfile == 0) {
    return;			/*file open failed */
  } else {
    while ((line = fgets(filename, sizeof(filename), formatsfile))) {
      if (*line == '#')
	continue;
      if (!strcspn(line, "\t\r\n"))
	continue;

      if (strcasecmp("formats", line) && (!formats_ok)) {
	formats_ok = TRUE;	/*set formats label found */
	continue;
      }

      if (!formats_ok) {
	continue;
      }
      /* only go on parsing 
         * when we indicate
         * we found a "formats" line 
       */
      //fprintf(stderr,"UADEformat: %s",line);

      if (strchr(line, '\t')) {	/*check if there's a tab */
	/*extract prefix/Extension */
	formatsline_l = g_strndup(line, strcspn(line, "\t"));

	/* extract playername */
	formatsline_r = strrchr(line, '\t');
	formatsline_r++;	/*skip left tab */
	*(formatsline_r + strcspn(formatsline_r, "\n\r")) = '\0';	/*get rid of \n and \r */

	p[0] = formatsline_l;
	p[1] = formatsline_r;
	gtk_clist_append(GTK_CLIST(formats_clist), p);
      }
    }
    fclose(formatsfile);
  }
}

static void clist_to_formatsfile_and_quit()
{
  clist_to_formatsfile();
  gtk_widget_destroy(formatswin);

}

static int clist_to_formatsfile()
{
  FILE *formatsfile;
  char filename[UADE_PATH_MAX];
  int rows;
  int i;
  gchar *p;

  /* use the uadepath from the entry and rather this instead of the saved one */
  //    resolve_path(formatsfilename, g_strdup(gtk_entry_get_text(GTK_ENTRY(uade_formats_path_entry))));

  strlcpy(filename, curr_formatsfilename, sizeof(filename));

//    fprintf (stderr, "saving pref/ext and players to: %s\n",curr_formatsfilename);
  formatsfile = fopen(filename, "w+");

  if (!formatsfile) {
    static GtkWidget *writeerrwin = NULL;

    if (!writeerrwin) {

      writeerrwin = xmms_show_message("Error writing UADEformats",
				      g_strdup_printf
				      ("ERROR writing file: \n\n"
				       "Could not create uadeformats file.\n"
				       "Please check if you have write permission on\n\n"
				       "%s\n\nand/or there is enough diskspace",
				       filename), "Close", FALSE, NULL,
				      NULL);
      gtk_signal_connect(GTK_OBJECT(writeerrwin), "destroy",
			 GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			 &writeerrwin);
    } else {
      gdk_window_raise(writeerrwin->window);
    }
    return FALSE;
  }

  /* write header */
  fprintf(formatsfile, "# File generated by the UADE xmms plugin.\n"
	  "# Edit at your own risk!\n\n" "formats\n");

  /* get number of rows of the list (0 - rows-1) */
  rows = GPOINTER_TO_INT(GTK_CLIST(formats_clist)->rows);
  if (rows != 0) {
    for (i = 0; i < rows; i++) {
      p = NULL;
      gtk_clist_get_text(GTK_CLIST(formats_clist), i, 0, &p);
      fprintf(formatsfile, "%s\t", p);

      p = NULL;
      gtk_clist_get_text(GTK_CLIST(formats_clist), i, 1, &p);
      fprintf(formatsfile, "%s\n", p);
    }
  }
  fprintf(formatsfile, "endformats\n");
  fclose(formatsfile);


  /* update uadeformat cache if neccessary */
  update_uadeformats_cache();

  return TRUE;
}

static void resolve_path(char *dst, char *src, int maxlen)
{
  if (maxlen <= 0)
    return;
  dst[0] = 0;
  if (src[0] == '~') {
    strlcpy(dst, getenv("HOME"), maxlen);
    src++;
  }
  strlcat(dst, src, maxlen);
}

#ifdef KDE2_MIMETYPE
static void formatsfile_to_kde2mime()
{
  FILE *formatsfile;
  FILE *kde2desktopfile;
  char filename[UADE_PATH_MAX];
  char kde2filename[UADE_PATH_MAX];
  char *line;
  char *formatsline_l;
  int ret;
  gboolean formats_ok = FALSE;
  static GtkWidget *kde2mimemsgwin = NULL;

  if (!uade_get_path(filename, UADE_PATH_FORMATSFILE, sizeof(filename))) {
    fprintf(stderr, "uade: formatsfile_to_kde2mime: could not get formatsfile\n");
    return;
  }

  strlcpy(curr_formatsfilename, filename, sizeof(curr_formatsfilename));

  resolve_path(kde2filename, KDE2_LOCALDIR, sizeof(kde2filename));	/* glue our filepath together */
  strlcat(kde2filename, "share/mimelnk/", sizeof(kde2filename));
  strlcat(kde2filename, KDE2_MIMETYPE, sizeof(kde2filename));
  strlcat(kde2filename, ".desktop", sizeof(kde2filename));

  formatsfile = fopen(filename, "r");
  if (formatsfile == 0) {
    return;			/*file open failed */
  } else {


    kde2desktopfile = fopen(kde2filename, "w+");

    if (!kde2desktopfile) {
      static GtkWidget *writeerrwin = NULL;

      if (!writeerrwin) {

	writeerrwin = xmms_show_message("Error writing KDE2 Desktop file",
					g_strdup_printf
					("ERROR writing file: \n\n"
					 "Could not create the KDE2 Desktop file.\n"
					 "Please check if you have write permission on\n\n"
					 "%s\n", kde2filename), "Close",
					FALSE, NULL, NULL);
	gtk_signal_connect(GTK_OBJECT(writeerrwin), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			   &writeerrwin);
      } else {
	gdk_window_raise(writeerrwin->window);
      }

      fclose(formatsfile);
      return FALSE;
    }

    /* write Entries */
    fprintf(kde2desktopfile,
	    "[Desktop Entry]\nComment=Amiga Music File\nHidden=false\nIcon=sound\nMimeType=audio/x-uade\n");

    fprintf(kde2desktopfile, "Patterns=");

    while ((line = fgets(filename, sizeof(filename), formatsfile))) {
      if (*line == '#')
	continue;
      if (!strcspn(line, "\t\r\n"))
	continue;

      if (strcasecmp("formats", line) && (!formats_ok)) {
	formats_ok = TRUE;	/*set formats label found */
	continue;
      }

      if (!formats_ok) {
	continue;
      }
      /* only go on parsing 
         * when we indicate
         * we found a "formats" line 
       */
      if (strchr(line, '\t')) {	/*check if there's a tab */
	/*extract prefix/Extension */
	formatsline_l = g_strndup(line, strcspn(line, "\t"));
	fprintf(kde2desktopfile, "%s.*;", formatsline_l);	/*write entry to desktopfile */

      }
    }
    fprintf(kde2desktopfile, "\nType=MimeType\n");
    fclose(kde2desktopfile);
    fclose(formatsfile);


    kde2mimemsgwin = xmms_show_message("KDE2 Filetype Creation",
				       g_strdup_printf
				       ("A KDE2 Desktop file containing\n"
					"the UADE Filetypes was successfully\n"
					"written to:\n%s\n", kde2filename),
				       "OK", FALSE, NULL, NULL);
    gtk_signal_connect(GTK_OBJECT(kde2mimemsgwin), "destroy",
		       GTK_SIGNAL_FUNC(gtk_widget_destroyed),
		       &kde2mimemsgwin);
    ret = system(KDE2_UPDATE);
  }
}
#endif

#ifdef ROX_MIMEINFO
static void formatsfile_to_roxmime()
{
  FILE *formatsfile;
  FILE *roxfile;
  char filename[UADE_PATH_MAX];
  char roxfilename[UADE_PATH_MAX];
  char *line;
  char *formatsline_l;
  char *formatsline_u;
  gboolean formats_ok = FALSE;
  static GtkWidget *roxmimemsgwin = NULL;

  if (!uade_get_path(filename, UADE_PATH_FORMATSFILE, sizeof(filename))) {
    fprintf(stderr, "uade: formatsfile_to_roxmime: could not get formatsfile\n");
    return;
  }
  strlcpy(curr_formatsfilename, filename, sizeof(curr_formatsfilename));

  resolve_path(roxfilename, ROX_MIMEINFO, sizeof(roxfilename));

  formatsfile = fopen(filename, "r");
  if (formatsfile == 0) {
    return;			/*file open failed */
  } else {


    roxfile = fopen(roxfilename, "w+");

    if (!roxfile) {
      static GtkWidget *writeerrwin = NULL;

      if (!writeerrwin) {

	writeerrwin = xmms_show_message("Error writing ROX MIME-info file",
					g_strdup_printf
					("ERROR writing file: \n\n"
					 "Could not create the ROX-MIME-info file.\n"
					 "Please check if you have write permission on\n\n"
					 "%s\n", roxfilename), "Close",
					FALSE, NULL, NULL);
	gtk_signal_connect(GTK_OBJECT(writeerrwin), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			   &writeerrwin);
      } else {
	gdk_window_raise(writeerrwin->window);
      }

      fclose(formatsfile);
      return;
    }

    /* write Entries */
    fprintf(roxfile,
	    "# ROX 1.2x MIME-info\n# generated by the uade xmms plugin\n# UADE: http://uade.ton.tut.fi\n\n");
    fprintf(roxfile, "%s\n", ROX_MIMETYPE);

    while ((line = fgets(filename, sizeof(filename), formatsfile))) {
      if (*line == '#')
	continue;
      if (!strcspn(line, "\t\r\n"))
	continue;

      if (strcasecmp("formats", line) && (!formats_ok)) {
	formats_ok = TRUE;	/*set formats label found */
	continue;
      }

      if (!formats_ok) {
	continue;
      }
      /* only go on parsing 
         * when we indicate
         * we found a "formats" line 
       */
      if (strchr(line, '\t')) {	/*check if there's a tab */
	/*extract prefix/Extension */
	formatsline_l = g_strndup(line, strcspn(line, "\t"));
	g_strdown(formatsline_l);

	formatsline_u = g_strdup(formatsline_l);
	g_strup(formatsline_u);

	/* write extension */
	fprintf(roxfile, "\text: %s\n", formatsline_l);

	/* write prefix */
	fprintf(roxfile, "\tregex: ^(%s|%s)\\..*\n\n", formatsline_l,
		formatsline_u);

      }
    }
    fclose(roxfile);
    fclose(formatsfile);


    roxmimemsgwin = xmms_show_message("ROX Filetype Creation",
				      g_strdup_printf
				      ("A ROX filemanager mimeinfo file containing\n"
				       "the UADE Filetypes was successfully\n"
				       "written to:\n%s\n", roxfilename),
				      "OK", FALSE, NULL, NULL);
    gtk_signal_connect(GTK_OBJECT(roxmimemsgwin), "destroy",
		       GTK_SIGNAL_FUNC(gtk_widget_destroyed),
		       &roxmimemsgwin);
  }
}
#endif

#ifdef SMI_MIMEINFO
static void formatsfile_to_smimime()
{
  FILE *formatsfile;
  FILE *smifile;
  char filename[UADE_PATH_MAX];
  char smifilename[UADE_PATH_MAX];
  char *line;
  char *formatsline_l;
  int ret;
  gboolean formats_ok = FALSE;
  static GtkWidget *smimimemsgwin = NULL;

  if (!uade_get_path(filename, UADE_PATH_FORMATSFILE, sizeof(filename))) {
    fprintf(stderr, "uade: formatsfile_to_smimime: couldn't get formatsfile\n");
    return;
  }
  strlcpy(curr_formatsfilename, filename, sizeof(curr_formatsfilename));

  resolve_path(smifilename, SMI_MIMEINFO, sizeof(smifilename));

  formatsfile = fopen(filename, "r");
  if (formatsfile == 0) {
    return;			/*file open failed */
  } else {


    smifile = fopen(smifilename, "w+");

    if (!smifile) {
      static GtkWidget *writeerrwin = NULL;

      if (!writeerrwin) {

	writeerrwin =
	    xmms_show_message("Error writing shared mime info file",
			      g_strdup_printf("ERROR writing file: \n\n"
					      "Could not create the shared mime info file.\n"
					      "Please check if you have write permission on\n\n"
					      "%s\n", smifilename),
			      "Close", FALSE, NULL, NULL);
	gtk_signal_connect(GTK_OBJECT(writeerrwin), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			   &writeerrwin);
      } else {
	gdk_window_raise(writeerrwin->window);
      }

      fclose(formatsfile);
      return;
    }

    /* write xml Header */
    fprintf(smifile, "<?xml version=\"1.0\"?>\n\n");
    fprintf(smifile,
	    "<!-- shared mime info file for AMIGA music files -->\n");
    fprintf(smifile,
	    "<!-- generated by the UADE xmms input plugin     -->\n");
    fprintf(smifile,
	    "<!-- UADE: http://uade.ton.tut.fi    -->\n\n");

    fprintf(smifile,
	    "<mime-info xmlns='http://www.freedesktop.org/standards/shared-mime-info'>\n");
    fprintf(smifile, "  <mime-type type=\"%s\">\n", SMI_MIMETYPE);
    fprintf(smifile, "\t<comment>Amiga Music file</comment>\n");

    while ((line = fgets(filename, sizeof(filename), formatsfile))) {
      if (*line == '#')
	continue;
      if (!strcspn(line, "\t\r\n"))
	continue;

      if (strcasecmp("formats", line) && (!formats_ok)) {
	formats_ok = TRUE;	/*set formats label found */
	continue;
      }

      if (!formats_ok) {
	continue;
      }
      /* only go on parsing 
         * when we indicate
         * we found a "formats" line 
       */
      if (strchr(line, '\t')) {	/*check if there's a tab */
	/*extract prefix/Extension */
	formatsline_l = g_strndup(line, strcspn(line, "\t"));
	g_strdown(formatsline_l);

	/* write extension */
	fprintf(smifile, "\t<glob pattern=\"*.%s\"/>\n", formatsline_l);

	/* write prefix */
	fprintf(smifile, "\t<glob pattern=\"%s.*\"/>\n", formatsline_l);

      }
    }
    fprintf(smifile, "  </mime-type>\n");
    fprintf(smifile, "</mime-info>\n");

    fclose(smifile);
    fclose(formatsfile);

    ret = system(SMI_UPDATE);	/*update mime db */

    smimimemsgwin = xmms_show_message("shared mime info generation",
				      g_strdup_printf
				      ("A shared mime info file containing\n"
				       "the UADE mime info was successfully\n"
				       "written to:\n%s\n", smifilename),
				      "OK", FALSE, NULL, NULL);
    gtk_signal_connect(GTK_OBJECT(smimimemsgwin), "destroy",
		       GTK_SIGNAL_FUNC(gtk_widget_destroyed),
		       &smimimemsgwin);
  }
}
#endif
