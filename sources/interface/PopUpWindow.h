#ifndef	POPUPWINDOW_H
#define POPUPWINDOW_H
#include <MediaNode.h>
#include <TextControl.h>
#include <Window.h>
#include <View.h>
#include "EventList.h"
#include <Menu.h>

class DrawApp;

class PopUpFiltre : public BView
{
public:
	PopUpFiltre(BRect frame);																				
	~PopUpFiltre();

	virtual void AttachedToWindow(void);
	virtual void Draw(BRect);

private:
	BButton			*Button;
	BButton			*filterButton;
	BTextControl	*Time;
	BTextControl	*End;
	menu_info		system;
	
};

class PopUpVideo : public BView
{
public:
	PopUpVideo(BRect frame);
	~PopUpVideo();
	
	virtual void AttachedToWindow(void);
	virtual void Draw(BRect);
	
private:
	BButton			*Button;
	BTextControl	*Begin;
	BTextControl	*Time;
	BTextControl	*End;
	menu_info		system;
};

class PopUpTransition : public BView
{
public:
	PopUpTransition(BRect frame);
	~PopUpTransition();
	
	virtual void AttachedToWindow(void);
	virtual void Draw(BRect);
	
private:
	BButton			*Button;
	BTextControl	*Time;
	BTextControl	*End;
	menu_info		system;
};

class PopUpWin : public BWindow
{
public:
	PopUpWin();
	~PopUpWin();
	
	virtual void MessageReceived(BMessage *message);
	void Adjust(EventComposant * Composant);
	
private:
	void 			InstantiateNode();
	void			HandleMediaMessage(BMessage *message);
	PopUpFiltre		*Filtre;
	PopUpVideo		*Video;
	PopUpTransition	*Transition;
	EventComposant	*Save;
	BMessage		*PopUpDraw;
	BView			*ParamView;
	BMediaNode		*node;
	bool			nodeWatched;
	BMediaRoster	*roster;
	BMessenger		*messenger;
	
friend PopUpFiltre;
friend PopUpVideo;
friend PopUpTransition;
friend DrawApp;
};

#endif
