#ifndef RUSHBOX_H
#define RUSHBOX_H
#include <Window.h>
#include <ListView.h>

#define RUSHBOX_HEIGHT	250
#define RUSHBOX_WIDTH	150

class rushwin;


class RushListView : public BListView
{
	public:
		RushListView(BRect frame, char *name);
		~RushListView();

		virtual bool InitiateDrag(BPoint point, int32 index, bool wasSelected);	
		virtual void KeyDown(const char *bytes, int32 numBytes);
		virtual void AttachedToWindow(void);
		virtual void FixupScrollBars();
		virtual void FrameResized(float width, float height);
	private: 
		rushwin		*parent;
};

class RushListItem : public BListItem
{
public: 
	RushListItem(entry_ref *filepath);
	~RushListItem();
	
	virtual void	DrawItem(BView *owner, BRect frame, bool complete = false);
	virtual void	Update(BView *owner, const BFont *font);
	status_t		InitCheck();
	const static float		height = 35.0; //hauteur de l'item
protected: 
	entry_ref	*media_path;
	BBitmap		*preview;
friend RushListView;
};

class rushwin : public BWindow
{
	public:
		rushwin();
		~rushwin();
		virtual void MessageReceived(BMessage *message);
	
	private:	
		RushListView *RushView; //liste etant affichée avec les noms des fichiers
	friend RushListView;
};
	
	
#endif
