/*
	
	HelloWindow.h
	
	Copyright 1997 Be Incorporated, All Rights Reserved.
	
*/

#ifndef DRAW_WINDOW_H
#define DRAW_WINDOW_H

#include <Window.h>
#include "MediaView.h"

class DrawWindow : public BWindow {
public:
					DrawWindow(BRect frame, const char *path); 
					~DrawWindow();
	virtual void 	MessageReceived(BMessage *message);
	
private: 
	MediaView *drawview;
};


#endif //DRAW_WINDOW_H
