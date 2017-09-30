#ifndef XMMS_UADE_DEFAULTS_H_
#define XMMS_UADE_DEFAULTS_H_

extern int use_timeout ;
extern char *timeout_val;
extern char *silence_timeout_val;
extern int force_by_default;
extern int use_ntsc;
extern int use_songend;
extern int lr_subsong_arrows;
extern int ud_subsong_arrows;
extern int fileinfo_on_play;
extern int use_slider;
extern int filemagic_check;
extern int filemagic_decr;
				     /* we need both of these */
extern int sequential_subsongs;	     /* used in GUI config */
extern int next_subsong_on_song_end; /* flag that only is set if  */ 
                                     /* use_timeout & use_songend */
				     /* with the value of sequential_subsongs*/
extern int next_subsong_on_timeout;  /* if non-zero, jump to next subsong on
				        timeout (see timeout_val) */

extern int do_mixing;
extern float mixing_parameter;

extern int do_volume_gain;
extern float volume_gain_parameter;

extern int use_filter;

extern int do_lp_filter;;
extern int lp_filter;		/* parameter 1-3*/

extern int auto_db_saves;
extern int auto_db_save_interval;

#endif
