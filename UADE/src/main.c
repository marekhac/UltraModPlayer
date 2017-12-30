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
#include "stdio.h"

#include <proto/dos.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include "api_MUI.h" 
#include "uade-main.h"

Object *app, *Win, *ModuleInfo, *PlayButton, *StopButton, *Img, *FileName;

struct Library *MUIMasterBase;

struct Hook Play_modules_hook;
struct Hook Stop_hook;

int myargc;
char **myargv;


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
				        Child, 
							FileName = MUI_NewObject (MUIC_String,
				          		MUIA_Frame, MUIV_Frame_String,
				        	End,
				      	End,	
					End, // end module info group
					
					// GROUP : CONTROL PANEL
					
					Child, HGroup, // control buttons panel
						GroupFrame,
		        		MUIA_Background, MUII_GroupBack,
		        		MUIA_Group_Rows, 1,
		 	    		MUIA_Group_Columns, 3,	
				
			 	   		Child, PlayButton = TextObject,
			         		MUIA_Frame,  MUIV_Frame_Button,
			         		MUIA_Background, MUII_ButtonBack,
			         		MUIA_Font, MUIV_Font_Button,
			         		MUIA_InputMode,  MUIV_InputMode_RelVerify,
			         		MUIA_Text_Contents,  (long)"\33cPlay",
			        	End,
					
				 	  	Child, StopButton = TextObject,
				        	MUIA_Frame,  MUIV_Frame_Button,
				         	MUIA_Background, MUII_ButtonBack,
				         	MUIA_Font, MUIV_Font_Button,
				         	MUIA_InputMode,  MUIV_InputMode_RelVerify,
				         	MUIA_Text_Contents,  (long)"\33cStop",
				        End,
					
				 	  	Child, TextObject,
				        	MUIA_Frame,  MUIV_Frame_Button,
				         	MUIA_Background, MUII_ButtonBack,
				         	MUIA_Font, MUIV_Font_Button,
				         	MUIA_InputMode,  MUIV_InputMode_RelVerify,
				         	MUIA_Text_Contents,  (long)"\33cExit",
				        End,
					End, // end of control buttons panel group
				End, // end VGroup
            End,
        End;
}

M_HOOK(Stop , APTR obj, APTR dana)
{
	printf("HOOK: Stop\n");
	
	stopProc();
}


M_HOOK(Play_modules , APTR obj, APTR dana)
{
   STRPTR FilenameStrPtr;

   GetAttr(MUIA_String_Contents, FileName, (ULONG*)&FilenameStrPtr);
    
   startNewProc(FilenameStrPtr);
}


void SetNotifications (void)
{
	DoMethod(Win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2,
	 MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	 
	DoMethod (PlayButton, MUIM_Notify, MUIA_Pressed, FALSE, app,
              2, MUIM_CallHook, &Play_modules_hook);
              
    DoMethod (StopButton, MUIM_Notify, MUIA_Pressed, FALSE, app,
              2, MUIM_CallHook, &Stop_hook);
                        
}


void MainLoop (void)
{
	ULONG signals = 0;

	set(Win, MUIA_Window_Open, TRUE);
			 
	while (DoMethod(app, MUIM_Application_NewInput, &signals) != MUIV_Application_ReturnID_Quit)
	{
		signals = Wait(signals | SIGBREAKF_CTRL_C);
		if (signals & SIGBREAKF_CTRL_C) break;
	}

	set(Win, MUIA_Window_Open, FALSE);

}

int main (int argc, char **argv)
{
 int i;

 myargc = argc;
 myargv = argv;
 
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