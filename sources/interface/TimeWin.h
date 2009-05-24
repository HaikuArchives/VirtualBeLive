#ifndef TIMEBOX_H
#define TIMEBOX_H

#include <Window.h>
#include <View.h>
#include <List.h>
#include "EventList.h"
#include "PopUpWindow.h"

/* Constants declarations  */
#define TIMEWIN_HEIGHT	150
const rgb_color	background_color = { 216, 216, 216, 255 };

/* Class decalarations */

class DrawApp;

class TimeView : public BView
{
public:
	TimeView(BRect frame);																				
	~TimeView();

	virtual void AttachedToWindow(void);
	virtual void Draw(BRect updateRect);
	virtual void FrameResized(float width, float height);
	virtual void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
	virtual void MessageReceived(BMessage *message);
	virtual void MouseDown(BPoint point);
	virtual void MouseUp(BPoint point);
	void KeyDown(const char *bytes, int32 numBytes);
	void MediaReceived(BMediaFile *media); // Parametres provisoire ...
	void Rescale();
private: 	
	bool in;
	void FixupScrollBars();
	bigtime_t	film_duration;
	int			scale;
	BRect		*TransitionRect;
	BRect		*VideoRect1;
	BRect		*VideoRect2;
	EventList	*List;
	EventComposant	*Composant;
	bool shift;
	bool resized;
	bool move;
	BPoint	before;
	BMessage	*Pop;
friend DrawApp;
};


#define CONTROLVIEW_WIDTH	120

class TimeControlView : public BView
{
public:
	TimeControlView(BRect frame);
	~TimeControlView();
	
	virtual void AttachedToWindow(void);
	virtual void Draw(BRect updateRect);
private:
	BSlider		*scaleSlider;
	
	friend class TimeView;
};

class TimeWin : public BWindow
{
public:
	TimeWin();
	~TimeWin();
	
	virtual void MessageReceived(BMessage *message);
	
private:

	TimeView			*tView;
	TimeControlView		*tControl;
friend DrawApp;
};

#endif