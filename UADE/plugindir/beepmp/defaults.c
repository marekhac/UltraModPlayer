
int use_timeout = 1;
int use_songend = 1;
int use_slider = 0;
char *timeout_val = "5:00";
char *silence_timeout_val = "20";
int force_by_default = 0;
int use_ntsc = 0;
int lr_subsong_arrows = 0;
int ud_subsong_arrows = 1;
int fileinfo_on_play = 0;
int filemagic_check = 1;
int filemagic_decr = 0;
int sequential_subsongs = 1;
int next_subsong_on_song_end = 1; /* see defaults.h for info */
int next_subsong_on_timeout = 1;

int do_mixing = 0;
float mixing_parameter = 0.7; /* range 0 - 1 */

int do_volume_gain = 0;
float volume_gain_parameter = 1.0; /* range 0 - inf */

int use_filter = 0; /* use amiga led filter emulation */

int do_lp_filter = 0; /* use post prcessing filter */
int lp_filter = 1; /* range 1 - 3: 1 least destructive, 3 most destructive */ 

int auto_db_saves = 1; /* true => save db automatically every now and then */
int auto_db_save_interval = 3600; /* auto save time interval in seconds */
