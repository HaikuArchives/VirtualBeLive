#ifndef TRANSITIONBOX_H
#define TRANSITIONBOX_H
#include <Window.h>
#include <ListView.h>
#include "EventList.h"

#define TRANSITIONBOX_HEIGHT	250
#define TRANSITIONBOX_WIDTH		160

class Transitwin;



class TransitListView : public BListView
{
	public:
		TransitListView(BRect frame, char *name);
		~TransitListView();

		virtual bool InitiateDrag(BPoint point, int32 index, bool wasSelected);
		virtual void AttachedToWindow(void);
		virtual void FrameResized(float width, float height);
	private:
		void FixupScrollBars();
		Transitwin	*parent;	
};

class TransitListItem : public BListItem
{
public:
	TransitListItem(char *transition_name, transition_type ftype);
	~TransitListItem();
	
	virtual void	DrawItem(BView *owner, BRect frame, bool complete = false);
	virtual void 	Update(BView *owner, const BFont *font);
	const static float		height = 35.0;
protected:
	char * transition_name;
	transition_type	type;
	BBitmap		*preview;
friend TransitListView;
};

class Transitwin : public BWindow
{
	public:
		Transitwin();
		~Transitwin();
	
	private:	
		TransitListView *TransitView; //liste etant affichée avec les noms des filtres et effets
	friend TransitListView;
};
	
	
#endif