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

Object *app, *Win, *ModuleInfo, *PlayButton, *StopButton, *SelectButton, *Img, *FileName, *Playlist;

// hooks

struct Hook Play_modules_hook;
struct Hook Stop_hook;
struct Hook Select_modules_hook;
struct Hook DataDestructor_hook;
struct Hook DataConstructor_hook;
struct Hook DataDisplayer_hook;

int myargc;
char **myargv;
int moduleIsPlaying = 0;

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

M_HOOK(Stop , APTR obj, APTR dana)
{
	stopProc();
	moduleIsPlaying = 0;
}


M_HOOK(Play_modules , APTR obj, APTR dana)
{
   	STRPTR FilenameStrPtr;
	long currentItem;
	struct Path *path = (struct Path *)dana;
 	char *filename = (char *)malloc(512 * sizeof(char));
	
 	currentItem = MUIV_List_NextSelected_Start; // position on the list

    DoMethod (Playlist, MUIM_List_NextSelected, &currentItem);
    DoMethod (Playlist, MUIM_List_GetEntry, currentItem, &path);
     
    strcpy(filename, path->drawer);
     
    if (AddPart(filename, path->filename, 512)) 
    {       	
    	stopProc();
	   	startNewProc(filename);
	}	
}


// Playlist hooks

// free memory after delete module from the list

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

		printf("\npathcopy: %s\n", pathCopy->filename);

       	return (long)pathCopy; 
    }
  	else
    {
       return NULL;
    }
}



struct WBArg *frArgs; 
char modulesDirectory[512]; 

#define MYWIDTH    320
#define MYHEIGHT   400

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
        else printf("User Cancelledn");

        CloseLibrary(AslBase);
    }
}

void SetNotifications (void)
{
	DoMethod (Win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2,
	 	 	  MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	 
	DoMethod (PlayButton, MUIM_Notify, MUIA_Pressed, FALSE, app,
              2, MUIM_CallHook, &Play_modules_hook);
              
    DoMethod (StopButton, MUIM_Notify, MUIA_Pressed, FALSE, app,
              2, MUIM_CallHook, &Stop_hook);
              
    DoMethod (SelectButton, MUIM_Notify, MUIA_Pressed, FALSE, Playlist,
   		      2, MUIM_CallHook, &Select_modules_hook);
                        
}


void MainLoop (void)
{
	ULONG signals = 0;

	set(Win, MUIA_Window_Open, TRUE);
			 
	while (DoMethod(app, MUIM_Application_NewInput, &signals) != MUIV_Application_ReturnID_Quit)
	{
		signals = Wait(signals | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F);
			if (signals & SIGBREAKF_CTRL_C) break;
		
			if (signals & SIGBREAKF_CTRL_F) {
				printf("SONG END!");		
			}
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