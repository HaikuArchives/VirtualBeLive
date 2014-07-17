#ifndef TRACKVIEW_H
#define TRACKVIEW_H

#include <Rect.h>
#include <View.h>

typedef enum {
	AUDIO_TYPE,
	VIDEO_TYPE,
	UNKNOWN_TYPE
} track_type;

class TrackView : public BView
{
public:
		TrackView(BRect frame, char*, track_type t);
		~TrackView();
		
	virtual void MessageReceived(BMessage *message);
	virtual void MouseMoved(BPoint point, uint32 transit, BMessage *message);
private:
	track_type	type;
};

#endif
