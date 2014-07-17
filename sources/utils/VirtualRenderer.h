#ifndef VIRTUAL_RENDERER_H
#define VIRTUAL_RENDERER_H

#include <Looper.h>

#include "EventList.h"
#include "AllNodes.h"
#include "ProjectPrefsWin.h"


class VirtualRenderer : public BLooper
{
public:
	VirtualRenderer(EventList *list);
	~VirtualRenderer();
	
	status_t	Start();
	void		MessageReceived(BMessage *msg);
protected:
static	int32	renderstart(void *castToVirtualRenderer);
	int32	RenderLoop();
private:

	void			FindFilter(filter_type type, media_node *node);
	void 			FindTransition(transition_type type, media_node * node);
	void			SetFilterParameters(media_node node, parameter_list *list);
	EventList		*eventList;
	thread_id		renderThread;
	sem_id			lock_sem;
	BMediaRoster	*roster;

	DiskWriter		*writer;
	FileReader		*reader1;
	FileReader		*reader2;
//	NullGen			*nullgen;
	
	Prefs			*prefs;
};



#endif