#ifndef _CONFIGURE_GUI_H_
#define _CONFIGURE_GUI_H_

void uade_about(); 
void uade_configure(); 
void uade_fileinfo(); 
void uade_playerinfo(); 
void uade_modinfo(); 

void uade_configread();
void uade_configwrite();

void uade_seeksubsong();

void uade_close_win();

void fileinfo_update();

void uade_alert(gchar *alerttext);

void processmodule(char *credits, char *filename, int credits_len);

/* COMMON DATA STRUCTURES */

extern gboolean has_subsong;
extern GtkWidget *configurewin;
extern GtkWidget *fileinfowin;
extern GtkWidget *seekpopup;

#endif
