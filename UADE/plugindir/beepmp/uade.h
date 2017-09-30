/* UADE - Unix Amiga Delitracker Emulator
 * 
 * Copyright (C) 2000-2004  Heikki Orsila
 *                          heikki.orsila@iki.fi
 *                          http://uade.ton.tut.fi
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
#ifndef XMMS_UADE_H
#define BEEP_UADE_H

#include <pthread.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>
#include <bmp/plugin.h>
#include <bmp/util.h>

#include "amifilemagic.h"

extern InputPlugin deli_ip;
extern char *uade_song_basename;
extern char *uade_song_full_name;
extern int uadeformats_is_cached;

extern void scanfile(char *filename, char *pre, char *post);

int is_paused(void);
void seek(int newsubsong, char *reason);
int get_curr_subsong(void);
int get_min_subsong(void);
int get_max_subsong(void);
char *get_playername(void);
char *get_playerfilename(void);
char *get_modulename(void);
char *get_formatname(void);
char *get_modulefilename(void);

void set_ntsc_pal(int use_ntsc);

void set_song_end_possible(gboolean use_songend);

#endif
