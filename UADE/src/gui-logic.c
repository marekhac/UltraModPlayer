#include "gui-logic.h"
#include "uade-main.h"

struct Process *childprocess = NULL;
UBYTE childprocessname[] = "childprocess";
BPTR output;
int quit = 0;
char *filename_strptr;
struct MsgPort *port;
	
void childprocesscode(void); 
void exitcode(void); 

struct userData
{
	BOOL playing;
};

void startNewProc(STRPTR filename)
{
	struct userData taskData = { .playing = TRUE };
	
	filename_strptr = filename;
   
    printf("StartNewProc\n");
    
    if (output = Open("*", MODE_OLDFILE))
    {
        if (childprocess = (struct Process *) CreateNewProcTags(
                          NP_Entry,       childprocesscode,  /* The child process  */
                          NP_Name,        childprocessname,
                          NP_StackSize,	  30000,
                          NP_CodeType, CODETYPE_PPC,
                          NP_UserData, &taskData,
                          TAG_END))

        {
            PutStr("Main Process: Created a child process\n");

        }
        else
            PutStr("Main Process: Can't create child process. Exiting.\n");
    }
    else
        PutStr("Main Process: Can't open CON:.  Exiting.\n");

}

void childprocesscode(void)  
{                        
	play();
}

void stopProc()
{

	if (childprocess)
	{
		
		BOOL playing = (BOOL)((struct userData*)childprocess->pr_Task.tc_UserData)->playing;
 		printf("stop proc: %d, address: %p\n",playing, childprocess);	
 		
		if(playing == TRUE)
	 	{
			port = CreateMsgPort();
	
			printf("stopping the uade\n");

			quit = 1;
			uae_quit();
			uade_reboot = 1;
			m68k_reset ();
	
			printf("waiting for message\n");
	
			WaitPort(port);

			printf("got message - uade resources released\n");
	
			DeleteMsgPort(port);
	
			printf("after DeleteMsgPort\n");
	
	 	 }
	}
}

void play()
{
   FILE *hf;
   STRPTR Tekst;
   char filename[250];
   char *arguments[2];
   int i;
   char first_arg[250] = "uade";   
   struct Message msg;
   struct userData taskData = { .playing = FALSE };
   BOOL playing;
   
   strcpy(filename,(char*)filename_strptr); 
   arguments[0] = first_arg;
   arguments[1] = filename; 
   
   printf("\nPLAY NEW MODULE\n");
       		
	default_prefs (&currprefs);
	uade_option (2, arguments);

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

	printf("play - before start_program\n");

	start_program ();
	
	printf("play - after start program\n");
	
	leave_program ();
	
	// wake up waiting stopProc
	
	if(port) 
	{
		printf("put message \n");
		PutMsg(port, &msg);
	}
	
	childprocess->pr_Task.tc_UserData = &taskData; 	
		
	playing = (BOOL)((struct userData*)childprocess->pr_Task.tc_UserData)->playing;
	printf("playing play: %d, address: %p\n",playing, childprocess);

}