#include "TrackView.h"

TrackView::TrackView(BRect frame, char *name, track_type t) : BView(frame, name, B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW)
{
	type = t;
}

TrackView::~TrackView()
{

}

void TrackView::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
		case msg_RushDropped:
			if (message.WasDropped())
				
				BPoint p = message->DropPoint();
				BRect	r = Bounds();
				r.left = p.x;
				r.right = 
			break;
	}
}

void TrackView::MouseMoved(BPoint point, uint32 transit, BMessage *message)
{

}