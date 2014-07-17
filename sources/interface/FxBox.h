#ifndef FXBOX_H
#define FXBOX_H
#include <Window.h>
#include <ListView.h>
#include "EventList.h"

#define FXBOX_HEIGHT	250
#define FXBOX_WIDTH		160

class Fxwin;

const rgb_color kHighLightColor = {215, 215, 255, 255};

class FxListView : public BListView
{
	public:
		FxListView(BRect frame, char *name);
		~FxListView();

		virtual bool InitiateDrag(BPoint point, int32 index, bool wasSelected);
		virtual void AttachedToWindow(void);
		virtual void FrameResized(float width, float height);
	private:
		void FixupScrollBars();
		Fxwin	*parent;	
};

class FxListItem : public BListItem
{
public:
	FxListItem(char *filter_name, filter_type ftype);
	~FxListItem();
	
	virtual void	DrawItem(BView *owner, BRect frame, bool complete = false);
	virtual void 	Update(BView *owner, const BFont *font);
	const static float		height = 35.0;
protected:
	char * filter_name;
	filter_type type;
	BBitmap		*preview;
friend FxListView;
};

class Fxwin : public BWindow
{
	public:
		Fxwin();
		~Fxwin();
	
	private:	
		FxListView *FxView; //liste etant affichée avec les noms des filtres et effets
	friend FxListView;
};
	
	
#endif