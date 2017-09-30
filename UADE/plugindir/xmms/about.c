/* XMMS UADE plugin
 *
 * Copyright (C) 2000-2003  Heikki Orsila
 *                          heikki.orsila@iki.fi
 *                          http://uade.ton.tut.fi
 *
 * This plugin is based on xmms 0.9.6 wavplayer input plugin code. Since
 * then all code has been rewritten.
 *
 * Initial gui code was based on code of the null-plugin by
 * Håvard Kvålen.
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
#include "gui.h"


//#include "uadelogo.xpm"


/* an About Dialog with GTK...
 * (basically a cloned Configure Dialog)
 *
 */

void uade_about(void)
{
  static GtkWidget *aboutwin = NULL;
//  static GtkWidget *pixmapwid = NULL;
//  static GdkPixmap *pixmap = NULL, *mask = NULL;

  GtkWidget *about_base_vbox;
  GtkWidget *about_notebook;
  //GtkStyle  *style;
  GtkWidget *about_txt_vbox;
  GtkWidget *about_txt;
  GtkWidget *uadeplay_txt_vbox;
  GtkWidget *uadeplay_scrolledwindow;
  GtkWidget *uadeplay_txt;
  GtkWidget *gpl_txt_vbox;
  GtkWidget *gpl_txt;
  GtkWidget *hint_txt_vbox;
  GtkWidget *hint_scrolledwindow;
  GtkWidget *hint_txt;

  GtkWidget *about_buttons_hbox, *ok_button; 


  if (!aboutwin){
  
  aboutwin = gtk_window_new(GTK_WINDOW_DIALOG);
  gtk_window_set_title(GTK_WINDOW(aboutwin), "Unix Amiga Deltracker Emulator");
  gtk_window_set_policy(GTK_WINDOW(aboutwin), FALSE, FALSE, FALSE);
  gtk_window_set_position(GTK_WINDOW(aboutwin), GTK_WIN_POS_MOUSE);
  gtk_container_set_border_width(GTK_CONTAINER(aboutwin), 10);
  
  about_base_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (aboutwin), about_base_vbox);

  about_notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (about_base_vbox), about_notebook, TRUE, TRUE, 0);

//Start of Page One

  about_txt_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(about_txt_vbox), 5);

/*
  gtk_widget_realize(aboutwin);
  
  pixmap = gdk_pixmap_create_from_xpm_d(aboutwin->window, &mask,
                                        NULL,uadelogo_xpm);   
  pixmapwid = gtk_pixmap_new(pixmap, mask);

  gtk_box_pack_start(GTK_BOX(about_txt_vbox), pixmapwid, FALSE, FALSE, 0 );
*/

  about_txt = gtk_label_new (     "Unix Amiga Delitracker Emulator\n"
                                  "http://uade.ton.tut.fi\n"
                                  "written by Heikki Orsila\n\n"
                                  "Plays Amiga music files by an UAE engine and\n"
                                  "an emulated deli/eagleplayer API.\n\n"
                                  "Use at own risk!\n"
				  "(and if it breaks you may keep all pieces ;)\n\n"
                                  "For bug reports, contributions and suggestions write to:\n"
                                  "Heikki Orsila <heikki.orsila@iki.fi>");
    gtk_box_pack_start(GTK_BOX(about_txt_vbox), about_txt, FALSE, FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(about_notebook), about_txt_vbox, gtk_label_new("UADE")); 

//End of Page 1

//Start of page two

  uadeplay_txt_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(uadeplay_txt_vbox), 5);

   uadeplay_scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
   gtk_container_add (GTK_CONTAINER (uadeplay_txt_vbox), uadeplay_scrolledwindow);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (uadeplay_scrolledwindow),GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

   uadeplay_txt = gtk_text_new (NULL, NULL);
   gtk_container_add (GTK_CONTAINER (uadeplay_scrolledwindow), uadeplay_txt);
   gtk_text_insert (GTK_TEXT (uadeplay_txt), NULL, NULL, NULL, 
		   "Thanks to:\n"
		   "\tEagleeye/Defect and Buggs/Defect for their kind\n"
		   "\tpermission to include their eagleplayers along with\n"
		   "\tuade...\n\n"
		   
		   "\tDon Adan and the Wanted Team for some of their\n"
		   "\teagleplayers, infos and help. Read their EP.*.readmes\n"
		   "\tor visit: http://www.emucamp.com/amiga/wt/wt.html\n\n"

		   "\tPaul v.d. Valk for permission to use his Medley\n"
		   "\treplay routine with uade\n\n"

		   "\tTap & Walt to let us use their digibooster example\n"
		   "\tsrc with uade.\n\n"
		   
		   "\tBrian Postma <b.postma@hetnet.nl> for placing his srcs\n"
		   "\tinto the Public Domain and for his permission b4 to\n"
		   "\tuse his sources for the bp3/soundmon2.2 replayer.\n"
		   "\tFor the sources and some tunes visit:\n"
		   "\thttp://www.homepages.hetnet.nl/~brianpostma\n\n"

		   "\tNicolas Pomarede <npomarede@kaptech.com> for his\n"
		   "\tpermission to distribute his MYST/YMST replayer\n"
		   "\talong with uade.\n"
		   "\tYou can find the complete MYST archive files with\n" 
		   "\tsongs and replays on Aminet at:\n"
		   "\thttp://aminet.net/~aminet/dirs/mus/play/MYST_*.lha\n\n"

    		   "\tStephen Mifsud (Malta) <teknologik@technologist.com>\n"
                   "\tfor making his Darius Zendeh replayer opensource.\n"
		   "\t(thanks!:)\n"
                   "\tFor further info about the DZ replayer and its license\n"
                   "\tcheck for the full source code and StephenMifsud.txt\n"
		   "\tin the amigasrc/players/dz/ of your uade source\n"
		   "\tdistribution. For his homepage go to:\n"
                   "\thttp://www.marz-kreations.com\n\n"

		   "\tBartman/Abyss for his permission to distribute their\n"
		   "\tAHX_v2 replayer by Dexter/Abyss along with uade.\n"
		   "\tVisit Abyss' homepage:\n"
		   "\thttp://www.the-leader-of-the-eighties.de\n\n"

		   "\tSean 'Odie' Connolly for releasing his nice EMS_v6\n"
		   "\treplay. Visit hishomepage:\n"
		   "\thttp://www.cosine-systems.com\n\n"

		   "\tNicolas Franck <gryzor@club-internet.fr> for his\n"
		   "\tpermission to use his brillant deli/prowiz converter\n"
		   "\twith uade!\n\n"
		   
		   "\tWhittaker playermodule V4.1 (27-Jan-95) adapted\n"
		   "\tby marley/INFECT\n\n"

		   "\tAndy Silva <silva@psi5.com> for his sources and\n"
		   "\tthe permission to use his players\n\n"

		   "\tSunBeam/Shelter for his replayers\n\n"

		   "\tLaurent Clévy and BuZz from the Exotica webpage,\n"
		   "\thttp://exotica.fix.no, for their help getting in contact\n"
		   "\twith authors of deli/eagleplayers, and for providing\n"
		   "\tsome additional players for testing.\n\n"

		   "\tClaudio Matsuoka and Hipolito Carraro Jr of the xmp\n"
		   "\tmodule player (http://xmp.sf.net). Uade's\n"
		   "\tmodule and player decrunching is based on their\n"
		   "\txmp unpack code with decr. routines by:\n"
		   "\t- jah@fh-zwickau.de (unsqsh),\n"
		   "\t- Sipos Attila (unsqsh chksum),\n"
		   "\t- Olivier Lapicque (mmcp),\n"
		   "\t- Marc Espie (old ppunpack)\n\n"

		   "\tDirk Stöcker for his valuable info and help on\n"
		   "\tdecrunching and detecting corrupt pp20 files and\n"
		   "\tGeorg Hörmann, the former pp20 detection is based on\n"
		   "\tcode by him.\n\n"

		   "\tKyzer - for the hint to the new pp20 decrunch/decrypt\n"
		   "\troutine.\n\n"

		   "\tMichael Doering - Plugin-GUI & Configuration, player\n"
		   "\ttesting, etc...\n\n"
		   
		   "\tXigh, Sylvain, and Matti Tiainen for getting BSD4.x\n"
		   "\tsupport on the way.\n\n"

		   "\tMeleth for bugfixes and suggestions concerning\n"
		   "\tthe compilationon Solaris/SunOS\n"
		   "\timprovenments of configure, installation, the \n"
		   "\tplayuade console wrapper, and maintining the\n"
		   "\tpwrap.pl script\n\n"

		   "\tMichael Baltaks for getting Mac OS-X Darwin\n"
		   "\tsupport on the way.\n\n"

		   "\tThe Mod  player is based on a replay by\n"
		   "\tThe Welder/Divine\n\n"
		    
		   "\tSylvain Chipaux for his valuable help, support, infos and\n"
		   "\ttesttunes on ptk and its clones. \n\n"

		    "\tDavid Olofson <david@olofson.net>: \n"
		    "\t(http://olofson.net) for the idea and\n"
		    "\tpatches related to ntsc/pal switching\n\n"

		    "\tLowpass filtering code is based\n"
		    "\ton code from tfmxplay by David 'Neochrome' Banz\n"
		    "\tand Jon 'Marx Marvelous' Pickard.\n\n"

		    "\tMikael Bouillot <mikael.bouillot@bigfoot.com>:\n"
		    "\tpatch to change subsongs with the xmms seek slider\n\n"

		   "\tAnd of course, all the UAE people for uae, their amiga\n"
		   "\temulator. Do read their docs/Credits and\n"
		   "\tdocs/README\n\n"

		   "Disclaimer:\n"
		   "\tEagleplayer is originally written by Eagleeye and\n\tBuggs of Defect\n\n"

		   "\tDelitracker is copyrighted and originally written by\n\tFrank Riffel and Peter Kunath.\n"
		   "\tDelitracker for Windows can be downloaded at:\n"
		   "\thttp://www.deliplayer.com.", -1);

   gtk_widget_set_usize(uadeplay_scrolledwindow, -1, 120);
   gtk_notebook_append_page(GTK_NOTEBOOK(about_notebook), uadeplay_txt_vbox, gtk_label_new("Credits")); 

//End of page two

//Start of page three
  hint_txt_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(hint_txt_vbox), 5);

   hint_scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
   gtk_container_add (GTK_CONTAINER (hint_txt_vbox), hint_scrolledwindow);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (hint_scrolledwindow),GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

   hint_txt = gtk_text_new (NULL, NULL);
   gtk_container_add (GTK_CONTAINER (hint_scrolledwindow), hint_txt);
   gtk_text_insert (GTK_TEXT (hint_txt), NULL, NULL, NULL, 
		   "HINTS:\n"
		   
		   "\tDo some players refuse to play the\n"
		   "\tmodules? Try enabling 'force player\n"
		   "\tto play the tunes you chose'\n\n"


		   "\tFor a very nice pseudo surround effect get\n"
		   "\tDavid Le Corfec's FX Toolbox plugin from\n"
		   "\tthe effects plugins page on the xmms\n"
		   "\thomepage [http://www.xmms.org].\n\n"

		   "\tFor a ported AHX and TFMX player get \n"
		   "\tDavid Le Corfec's plugins from the\n"
		   "\tinput plugins page on the xmms\n"
		   "\thomepage.\n\n"

		   "\tFor playing a lot of Protracker clones get\n"
		   "\tyourself XMP from http://xmp.sf.net\n"
		   "\tBTW. XMP has also an experimental xmms plugin\n\n"
		   
		   "\tFor a native soundmon2.0/2.2 player for\n"
		   "\tLinux and Solaris written by Brian Postma\n"
		   "\tvisit his homepage:\n"
		   "\thttp://www.homepages.hetnet.nl/~brianpostma\n\n"

		   "\tA Futurecomposer reference player for Unix\n"
		   "\tby Michael Schwendt is available on the\n"
		   "\txmms input plugins page\n\n"

		    "\tFor ripping Amiga music yourself on a PC class\n"
		    "\tmachine get yourself prowiz for pc by Sylvain 'Asle'\n"
		    "\tChipaux\n\n"

		   "\tFor live streaming your amiga tunes get\n"
		   "\tthe LiveIce plugin from the xmms homepage\n\n"
		   
		   "\tFor heaps of Amiga tunes and nostalgia visit\n"
		   "\tthe Exotica Webpage at: http://exotica.fix.no\n\n"
		   
		   "\tFor more info on Amiga fileformats and replays\n"
		   "\tvisit: http://perso.club-internet.fr/lclevy/exotica\n\n"

		    "\tFor live streamed Amiga Demo Scene music go to:\n"
		    "\thttp://nectarine.ojuice.net\n\n"

		    "\t...or listen to Kohina - Pure old school 8bit & 16bit\n"
		    "\tgame and demo music mp3/ogg radio at:\n"
		    "\thttp://www.kohina.com\n\n"
		   , -1);

   gtk_widget_set_usize(hint_scrolledwindow, -1, 120);
   gtk_notebook_append_page(GTK_NOTEBOOK(about_notebook), hint_txt_vbox, gtk_label_new("Hints and Tips")); 

//End of page three


  gpl_txt_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(gpl_txt_vbox), 5);



  gpl_txt = gtk_label_new ("This program is free software; you can redistribute it and/or\n"
                           "modify it under the terms of the GNU General Public License\n"
			   "as published by the Free Software Foundation; either version 2\n"
			   "of the License, or (at your option) any later version.\n\n"

                            "This program is distributed in the hope that it will be useful,\n"
			   "but WITHOUT ANY WARRANTY; without even the implied warranty\n"
                           "of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"  
                           "See the GNU General Public License for more details.\n\n"

                           "You should have received a copy of the GNU General Public License\n"
			   "along with this program; if not, write to the Free Software\n"
			   "Foundation, Inc., 59 Temple Place - Suite 330, Boston\n"
			   "MA 02111-1307, USA");

    gtk_box_pack_start(GTK_BOX(gpl_txt_vbox), gpl_txt, FALSE, FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(about_notebook), gpl_txt_vbox, gtk_label_new("GPL")); 


//End of page four



  about_buttons_hbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(about_buttons_hbox), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(about_buttons_hbox), 5);
  gtk_box_pack_start(GTK_BOX(about_base_vbox), about_buttons_hbox, FALSE, FALSE, 0);

  ok_button = gtk_button_new_with_label("Ok");
  GTK_WIDGET_SET_FLAGS(ok_button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object(GTK_OBJECT(ok_button), "clicked",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(aboutwin));
  gtk_widget_grab_default(ok_button);
  gtk_box_pack_start_defaults(GTK_BOX(about_buttons_hbox), ok_button);



  gtk_widget_show_all(aboutwin);



  gtk_signal_connect(GTK_OBJECT(aboutwin), "destroy",
		     GTK_SIGNAL_FUNC(gtk_widget_destroyed),
		     &aboutwin);
  }
  else
      {
      gdk_window_raise (aboutwin->window);
      }
}
/* End of about GTK Code*/






