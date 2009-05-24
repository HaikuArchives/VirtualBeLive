#ifndef DRAW_H
#define DRAW_H

#include <Application.h>
#include "MenuWindow.h"
#include "rushbox.h"
#include "FxBox.h"
#include "PopUpWindow.h"
#include "TransitionBox.h"
#include "TimeWin.h"
#include "consts.h"
#include "AllNodes.h"
#include "ProjectPrefsWin.h"
#include "VirtualRenderer.h"

class DrawApp : public BApplication {
public:
					DrawApp();
	virtual			~DrawApp();
	
	void			OpenSelectPanel();
	void			OpenOpenPanel();
	void			OpenSavePanel();
	void			OpenOpenProjectPanel();
	void			Splash();

	virtual void	ReadyToRun();
	virtual void	ArgvReceived(int32 argc, char **argv);
	virtual status_t Save(BMessage *message);
	virtual void	MessageReceived(BMessage *message);
	virtual	void	RefsReceived(BMessage *message);

	virtual void	PlayFile(const char *path);
	virtual bool	QuitRequested();
private:
	BFilePanel*		fSelectPanel;
	BFilePanel*		fOpenPanel;
	BFilePanel*		fSavePanel;
	BFilePanel*		fOpenProjectPanel;
	MenuWindow*		fMenuWin;
	Transitwin*		TransitBox;
	rushwin*		Rush;
	Fxwin*			FxBox;
	PopUpWin*		PopUp;
	TimeWin*		TimeBox;
	BMediaRoster	*roster;
	BTimeSource		*timeSource;
		
	DiskWriter		*writer;
	FileReader		*reader;
	FileReader		*reader2;
	TestTransition	*transition;
	ProjectPrefsWin	*prefsWin;
	media_node		dispNode;

	VirtualRenderer	*renderer;
	static int32	sNumWindows;
	Prefs			prefs;
friend VirtualRenderer;
};


#endif //DRAW_H
