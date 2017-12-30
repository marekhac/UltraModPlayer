#include "playmodule.h"
#include "uade-main.h"

struct Process *childprocess = NULL;
UBYTE childprocessname[] = "childprocess";


BPTR output;
int quit = 0;
char *filename_strptr;
   
void childprocesscode(void); 

void startNewProc(STRPTR filename)
{
	filename_strptr = filename;

    quit = 0; 
    
    if (output = Open("*", MODE_OLDFILE))
    {
        if (childprocess = (struct Process *) CreateNewProcTags(
                          NP_Entry,       childprocesscode,  /* The child process  */
                          NP_Name,        childprocessname,
                          NP_CodeType, CODETYPE_PPC,
                          TAG_END))

        {
            PutStr("Main Process: Created2 a child process");
            PutStr(" and waiting on SIGF_SINGLE.\n");

            Flush(Output());

            SetSignal(0L, SIGF_SINGLE);

            Signal((struct Task *)childprocess,SIGBREAKF_CTRL_F);

        }
        else
            PutStr("Main Process: Can't create child process. Exiting.\n");
    }
    else
        PutStr("Main Process: Can't open CON:.  Exiting.\n");

}

void childprocesscode(void)     /* This function is what CreateNewProcTags() */
{                               /* loads as the child process.  This child   */
                                /* signals the parent using SIGF_SINGLE.     */
    ULONG x;

    /* Wait for a startup signal. This is to allow the parent process to
     * print its banner message and clear SIGF_SINGLE.
     */
    Wait(SIGBREAKF_CTRL_F);

    PutStr("Child Process: I'm alive and starting a 5 second TimeDelay()");
    Flush(Output());


	play();
/*
    while (quit == 0)
    {


		printf(".");
        //FPuts(Output(), ".");           
        Flush(Output());
    }
	*/ 
	//Signal((struct Task *)muiprocess, SIGF_SINGLE);    /* Finished waiting, */

	//childprocess = NULL;
}

void stopProc()
{
	printf("Stop\n");
	quit = 1;
	//uae_quit();
	uade_reboot = 1;
	//m68k_reset ();
	//leave_program ();
	/*
	childprocess = NULL;
	
		  while(childprocess != NULL) // wait to be finished
	  {
		  printf("Ubijam subproces\n");
	  
	 }
	 */
	 
	 printf("-> end of stopProc");
} 

void play()
{

	//printf("play module\n");
	

	   FILE *hf;
   STRPTR Tekst;
   char filename[250];
   char *arguments[2];
   int i;
   char first_arg[250] = "uade";   
   //char filename[250] = "test.mod";
      
   //GetAttr(MUIA_String_Contents, FileName, (ULONG*)&Tekst);

   strcpy(filename,(char*)filename_strptr); 
   arguments[0] = first_arg;
   arguments[1] = filename; 
   
   
   //printf("filename %s\n", Tekst);
   
     for (i = 0; i < 2; i++)
  {
  	printf("arg2: %d = %s\n", i, arguments[i]);
  }
   
    //play();
    
    // -------------------
  		
	default_prefs (&currprefs);
	uade_option (2, arguments);

	printf("WITAMY W ULTRAMODPLAYER\n");	
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

	custom_init (); // Must come after memory_init 

	reset_frame_rate_hack ();
	init_m68k(); // must come after reset_frame_rate_hack (); 

	// compiler_init (); 

	if (currprefs.start_debugger)
		activate_debugger ();

	printf("-> before start_program");

	start_program ();

	printf("-> after start program");
	

	//leave_program ();
	
//	return 0;
	
}