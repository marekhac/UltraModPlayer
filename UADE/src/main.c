 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Main program
  *
  * Copyright 1995 Ed Hanway
  * Copyright 1995, 1996, 1997 Bernd Schmidt
  */

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>

#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <clib/asl_protos.h>
#include "stdio.h"

#include <proto/dos.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
 
#include "api_MUI.h" 
#include "uade-main.h"
#include "gui-path.h"
#include "amiga-shell.h"
#include "main.h"

#define MYWIDTH   320
#define MYHEIGHT  400

Object *app, *Win, *PlayButton, *NextButton, *StopButton, *SelectButton, *Img, *FileName, *Playlist;

// hooks
 
struct Hook Play_modules_hook;
struct Hook Stop_hook;
struct Hook Select_modules_hook;
struct Hook DataDestructor_hook;
struct Hook DataConstructor_hook;
struct Hook DataDisplayer_hook;
struct Hook NextModule_hook;
 
struct MsgPort *port;
struct Task *mainTask = NULL;
struct Process *childprocess = NULL;

LONG uadeExitSignalNumber = -1;
ULONG uadeExitSignal;
BOOL moduleStopedByUser = 0;
UBYTE childprocessname[] = "childprocess";
BPTR output;
char *filename_strptr;

void childprocesscode(void); 
struct userData
{
	BOOL playing;
};

void startNewProc(STRPTR);
void play(void);
void stopProc(void);
void playNextModule(void);

struct WBArg *frArgs; 
char modulesDirectory[512]; 

struct Library *AslBase = NULL;

struct TagItem frtags[] =
{
    ASL_Hail,       (ULONG)"Choose modules to play",
    ASL_Height,     MYHEIGHT,
    ASL_Width,      MYWIDTH,
    ASL_OKText,     (ULONG)"Add to list",
    ASL_CancelText, (ULONG)"Cancel",
    ASL_Dir,        (ULONG)modulesDirectory,
    ASL_File,       (ULONG)"",
    ASL_FuncFlags, FILF_MULTISELECT | FILF_PATGAD,
    TAG_DONE
};


long BuildApplication (void)
{
	app = ApplicationObject,
		MUIA_Application_Author, (ULONG)"Marek Hac",
		MUIA_Application_Base, (ULONG)"ULTRAMODPLAYER",
		MUIA_Application_Copyright, (ULONG)"??? 2017 Marek Hac / MarX",
		MUIA_Application_Title, (ULONG)"UltraModPlayer",
		SubWindow,
            Win = WindowObject,
                MUIA_Window_Title, (ULONG)"UltraModPlayer",
                WindowContents,
                VGroup,
				
					// GROUP : MODULE INFO
				
					Child, HGroup,
                    	GroupFrame,
                    	MUIA_Background, MUII_GroupBack,
                   	 	MUIA_Group_Rows, 1,
                    	MUIA_Group_Columns, 2,
    
						Child, HGroup, 
				        	Child, TextObject,
				          		MUIA_Text_Contents, (long)"\33rFile:", /* justify right */
				          	  	MUIA_Text_SetMax, TRUE,
				         	End,
				            Child, FileName = TextObject,    
        						MUIA_Frame, MUIV_Frame_Text,
        						MUIA_Background, MUII_TextBack,			         			
       						End,
       					End, 	
					End, // end module info group
					
					// GROUP : CONTROL PANEL
					
					Child, HGroup, // control buttons panel
						GroupFrame,
		        		MUIA_Background, MUII_GroupBack,
		        		MUIA_Group_Rows, 1,
		 	    		MUIA_Group_Columns, 4,	
				
			 	   		Child, PlayButton = TextObject,
			         		MUIA_Frame,  MUIV_Frame_Button,
			         		MUIA_Background, MUII_ButtonBack,
			         		MUIA_Font, MUIV_Font_Button,
			         		MUIA_InputMode,  MUIV_InputMode_RelVerify,
			         		MUIA_Text_Contents,  (long)"\33cPlay",
			        	End,
					
						Child, NextButton = TextObject,
			         		MUIA_Frame,  MUIV_Frame_Button,
			         		MUIA_Background, MUII_ButtonBack,
			         		MUIA_Font, MUIV_Font_Button,
			         		MUIA_InputMode,  MUIV_InputMode_RelVerify,
			         		MUIA_Text_Contents,  (long)"\33cNext",
			        	End,
					
				 	  	Child, StopButton = TextObject,
				        	MUIA_Frame,  MUIV_Frame_Button,
				         	MUIA_Background, MUII_ButtonBack,
				         	MUIA_Font, MUIV_Font_Button,
				         	MUIA_InputMode,  MUIV_InputMode_RelVerify,
				         	MUIA_Text_Contents,  (long)"\33cStop",
				        End,
					
				 	  	Child, SelectButton = TextObject,
				        	MUIA_Frame,  MUIV_Frame_Button,
				         	MUIA_Background, MUII_ButtonBack,
				         	MUIA_Font, MUIV_Font_Button,
				         	MUIA_InputMode,  MUIV_InputMode_RelVerify,
				         	MUIA_Text_Contents,  (long)"\33cSelect",
				        End,
					End, // end of control buttons panel group
					
					Child, HGroup,
        				GroupFrame,
        				MUIA_Background, MUII_GroupBack,
        				MUIA_Group_Rows, 2,
	    				MUIA_Group_Columns, 1,
		
      					Child, Playlist = ListviewObject,
      						MUIA_Listview_List, ListObject,
       							MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
       							MUIA_List_DestructHook, MUIV_List_DestructHook_String,
       							MUIA_Frame, MUIV_Frame_ReadList,
	   							MUIA_Listview_DragType, MUIV_Listview_DragType_Immediate,
	   							MUIA_List_Format, "COL=0,P=\33c",
	  
	   							// control of the playlist

       							MUIA_List_ConstructHook, (long)&DataConstructor_hook,
       							MUIA_List_DestructHook, (long)&DataDestructor_hook,
       							MUIA_List_DisplayHook, (long)&DataDisplayer_hook,
       						End,	
	  					End,
     				End,
				End, // end VGroup
            End,
        End;
}

// HOOKs

M_HOOK(Stop, APTR obj, APTR dana)
{
	stopProc();
}

M_HOOK(Play_modules, APTR obj, APTR dana)
{
	playSelectedModule();
}

// GUI CONTROL

void update_gui_filename(char *filename) 
{		
	DoMethod (app, MUIM_Application_PushMethod, FileName, 4, MUIM_SetAsString, MUIA_Text_Contents, "%ls", filename);
}

void setUadeExitFlags()
{
	uade_reboot = 1;
}

void quitChildProcess()
{
	Signal((struct Task *)childprocess, SIGBREAKF_CTRL_C);
	childprocess = NULL;
} 

void playNextModule()
{
 	printf("Play next module\n");
 	
	setUadeExitFlags();
		
	if (childprocess)
	{
		printf("after DeleteMsgPort\n");

		quitChildProcess();
	}

	nextModule();
}


void playSelectedModule() 
{
	long currentItem;
	struct Path *path;
	char *filename;
	
	// AllocTaskPooled is freed automatically when the process ends
	
	path = AllocTaskPooled(sizeof(struct Path));
	filename = (char *)AllocTaskPooled(512 * sizeof(char));
	
 	currentItem = MUIV_List_NextSelected_Start;

    DoMethod (Playlist, MUIM_List_NextSelected, &currentItem);
    DoMethod (Playlist, MUIM_List_GetEntry, currentItem, &path);
     
    strcpy(filename, path->drawer);
     
    if (AddPart(filename, path->filename, 512)) 
    {       	
    	stopProc();
	   	startNewProc(filename);
	}	
}

void nextModule()
{

	long currentItem;         
	long nextItem;
	long itemsCounter;	   
 
   	get(Playlist,MUIA_List_Entries,&itemsCounter);

   	if (itemsCounter>0) 
   	{

   		char *filename = (char *)malloc(512 * sizeof(char)); // allocmem, alloctaskpooled (mos)
   		currentItem = MUIV_List_NextSelected_Start; 
   
   		get(Playlist,MUIA_List_Active,&currentItem); // take current one
   
   		nextItem = currentItem + 1;
        
   		set(Playlist,MUIA_List_Active,(nextItem)%itemsCounter); // select next item in playlist
      
   		playSelectedModule(); // get item details and play it

  	}  
  	else
  	{
  		printf("no modules to play\n");
   	}
}

M_HOOK(Next_module, APTR obj, APTR dana) 
{
	nextModule();
}

// playlist hooks 


M_HOOK(DataDestructor, APTR obj, APTR data) 
{
	playlistDestructor(obj,data);
}

// tells what (and how) items should be displayed inside the listview

M_HOOK(DataDisplayer, APTR obj, APTR data) 
{
	playlistDisplayer(obj, data);
}

M_HOOK(DataConstructor, APTR obj, APTR data) 
{
	APTR mempool = (APTR)obj;
 	struct Path *path = (struct Path *)data;
 	
 	char *filenameCopy;
 	char *drawerCopy;

 	struct Path *pathCopy;

	// alloc memory from pool for filename
		
  	if (filenameCopy = AllocPooled (mempool, strlen((char *) path->filename)+1))
  	{
  		// make a copy of filename
  	
    	strcpy(filenameCopy,path->filename);
  	} 
  
    // alloc memory from pool for drawer
  
  	if (drawerCopy = AllocPooled (mempool, strlen((char *) path->drawer)+1))
  	{
  		// make a copy of drawer
  		
	    strcpy(drawerCopy,path->drawer);
	}

	// alloc memory from pool for complete path

	if ((pathCopy = AllocPooled (mempool, sizeof (struct Path))))
    {
    	// create a complete path
    
        pathCopy->filename = filenameCopy;
        pathCopy->drawer = drawerCopy;

		printf("\n -> path copy: %s\n", pathCopy->filename);

       	return (long)pathCopy; 
    }
  	else
    {
       return NULL;
    }
}

M_HOOK(Select_modules , APTR obj, APTR dana) 
{

 LONG fileNumber;  
 struct FileRequester *fr;
 struct Path path;
 char filename[512];


 	if(!modulesDirectory[0])
  	{
  	  // Try to obtain the current directory name.
	  // without this, ASL will report empty dir as current dir
	  
  	  if(!GetCurrentDirName(modulesDirectory,512))
          modulesDirectory[0] = 0;
  	}
 
  	if (AslBase = OpenLibrary("asl.library", 37L))
    {
        if (fr = (struct FileRequester *)
            AllocAslRequest(ASL_FileRequest, frtags))
        {
            if (AslRequest(fr, NULL))
            {
            	
            	// rf_ArgList is an array of WBArg structures
                // Each entry in this array corresponds to one of the files
                // the user selected (in alphabetical order).
            	
            	frArgs = fr->rf_ArgList;
                        	
                strcpy(modulesDirectory,fr->rf_Dir); 

                // temporarily prevent the list from being refreshed    
                
                set (Playlist, MUIA_List_Quiet,TRUE);
                
                for (fileNumber=0; fileNumber < fr->rf_NumArgs; fileNumber++)
	      		{
	       			path.filename = frArgs[fileNumber].wa_Name;
               		path.drawer = fr->rf_Dir;
  	 	    
  	 	    		printf("\nfull path: %s%s\n", path.drawer, path.filename);
  	 	    
                	DoMethod (Playlist, MUIM_List_InsertSingle, &path, MUIV_List_Insert_Bottom);
              }
                
                set (Playlist, MUIA_List_Quiet,FALSE);
            }
            FreeAslRequest(fr);
        }
        else printf("User Cancelled\n");

        CloseLibrary(AslBase);
    }
}

void SetNotifications (void)
{
	DoMethod (Win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2,
	 	 	  MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	 
	DoMethod (PlayButton, MUIM_Notify, MUIA_Pressed, FALSE, app,
              2, MUIM_CallHook, &Play_modules_hook);
              
    DoMethod (NextButton, MUIM_Notify, MUIA_Pressed, FALSE, Playlist,
   		   2, MUIM_CallHook, &Next_module_hook);	
              
    DoMethod (StopButton, MUIM_Notify, MUIA_Pressed, FALSE, app,
              2, MUIM_CallHook, &Stop_hook);
              
    DoMethod (SelectButton, MUIM_Notify, MUIA_Pressed, FALSE, Playlist,
   		      2, MUIM_CallHook, &Select_modules_hook);
                        
}

void MainLoop (void)
{
	ULONG signals = 0;
	BOOL running = TRUE;
	
	set(Win, MUIA_Window_Open, TRUE);
			 	 
	while (DoMethod(app, MUIM_Application_NewInput, &signals) != MUIV_Application_ReturnID_Quit)
	{
		signals = Wait(signals | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F | uadeExitSignal); 
		
		if (signals & SIGBREAKF_CTRL_C) {
			break;
		}			
		
		if (signals & uadeExitSignal)
		{
			printf("signal received \n");
			playNextModule();				
		}
	}
	
	printf("while loop end \n");
	
	stopProc();
	
	printf("going to quit the app \n");

	set(Win, MUIA_Window_Open, FALSE);

}

int main (int argc, char **argv)
{
 int i;
   
 uadeExitSignalNumber = AllocSignal(-1);
 uadeExitSignal = 1L << uadeExitSignalNumber;
 
 mainTask = FindTask(NULL);
 
  for (i = 0; i < argc; i++)
  {
  	printf("arg: %d = %s\n", i, argv[i]);
  }
  
  printf("argc: %d",argc);


  if (IntuitionBase = (struct IntuitionBase*)OpenLibrary ("intuition.library", 37))
   {
    if (MUIMasterBase = (struct MUIMasterBase*) OpenLibrary ("muimaster.library", 19))
     {
      if (BuildApplication ())
       {
         SetNotifications ();
         MainLoop ();
         MUI_DisposeObject (app);
       }
      CloseLibrary (MUIMasterBase);
     }
    CloseLibrary ((struct Library*)IntuitionBase);
   }
  return 0;   

}

// ------------------------------------------------------------


void startNewProc(STRPTR filename)
{
	filename_strptr = filename;

    printf("StartNewProc \n");
    
	// startup msg

    if (childprocess = (struct Process *) CreateNewProcTags(
                        NP_Entry,       childprocesscode,  
                        NP_Name,        childprocessname,
                        NP_Priority, 	1,
                        NP_CodeType, 	CODETYPE_PPC,
                       TAG_DONE))

    {
    	PutStr("Main Process: Created a child process\n");    	
    }
    else
        PutStr("Main Process: Can't create child process. Exiting.\n");

}

void childprocesscode(void)  
{          
    BPTR nil = Lock("NIL:", ACCESS_WRITE);
    
    //SetOutput(nil); // printf from child process goes to nil
	
	play();
	
	//UnLock(nil);
}

void stopProc()
{
	setUadeExitFlags();
	
	
	if (childprocess)
	{
		if (m68k_check)
		{		
			moduleStopedByUser = 1;
		
			port = CreateMsgPort();
	
			printf("stopping the uade\n");
			printf("waiting for message\n");
	
			WaitPort(port);

			printf("got message - uade resources released\n");
	
			DeleteMsgPort(port);
	
			printf("after DeleteMsgPort\n");
			
			quitChildProcess();
			
			moduleStopedByUser = 0;
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
	
	childprocess->pr_Task.tc_UserData = &taskData; 	
		
	playing = (BOOL)((struct userData*)childprocess->pr_Task.tc_UserData)->playing;
	printf("playing play: %d, address: %p\n",playing, childprocess);
	
	// wake up waiting stopProc
		
	if (!moduleStopedByUser) 
	{
		printf("send signal to play next module \n");
		Signal(mainTask, uadeExitSignal);
	}
	else
	{
		printf("put message \n");
		PutMsg(port, &msg);
	}
}