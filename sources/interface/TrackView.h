#ifndef TRACKVIEW_H
#define TRACKVIEW_H


enum track_type(
	AUDIO_TYPE,
	VIDEO_TYPE,
	UNKNOWN_TYPE
	}

class TrackView : public BView
{
public:
		TrackView(BRect frame, track_type t);
		~TrackView();
		
	virtual void MessageReceived(BMessage *message);
	virtual void MouseMoved(BPoint point, uint32 transit, BMessage *message);
private:
	track_type	type;
};

#endif