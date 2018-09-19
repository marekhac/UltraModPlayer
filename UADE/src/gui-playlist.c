#include "gui-playlist.h"
#include "gui-path.h"

void playlistDestructor(APTR mempool, APTR data)
{
	struct Path *path = (struct Path *)data;

	if (path)
    {
    	 FreePooled (mempool, path, sizeof (struct Path));
    }
}
   
void playlistDisplayer(APTR obj, APTR data)
{
 struct Path *path = (struct Data *)data;
 char **playlistItem = (char **)obj;
 
 playlistItem[0] = path->filename; // push only filename to the playlist
}

void playlistConstructor(APTR obj, APTR data) 
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

       	return (long)pathCopy; 
    }
  	else
    {
       return NULL;
    }
}