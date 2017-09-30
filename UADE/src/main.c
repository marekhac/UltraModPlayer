 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Main program
  *
  * Copyright 1995 Ed Hanway
  * Copyright 1995, 1996, 1997 Bernd Schmidt
  */

#include "uade-main.h"

int main (int argc, char **argv)
{
    FILE *hf;
		
	default_prefs (&currprefs);
	uade_option (argc, argv);
	
	machdep_init ();

	if (! setup_sound ()) {
		fprintf (stderr, "Sound driver unavailable: Sound output disabled\n");
		currprefs.produce_sound = 0;
		uade_exit(-1);
	}

	if (sound_available && currprefs.produce_sound > 1 && ! init_sound ()) {
		fprintf (stderr, "Sound driver unavailable: Sound output disabled\n");
		currprefs.produce_sound = 0;
		uade_exit(-1);
	}
	
	fix_options ();
	changed_prefs = currprefs;
	check_prefs_changed_audio();
	check_prefs_changed_cpu();

	memory_init ();

	custom_init (); /* Must come after memory_init */

	reset_frame_rate_hack ();
	init_m68k(); /* must come after reset_frame_rate_hack (); */

	/* compiler_init (); */

	if (currprefs.start_debugger)
		activate_debugger ();

	start_program ();

	leave_program ();

	return 0;
}