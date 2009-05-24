#include "draw_window.h"
#include "draw.h"
#include "MediaView.h"
#include <Path.h>


DrawWindow::DrawWindow(
	BRect		frame, 
	const char	*path)
		: BWindow(frame, "Draw", B_TITLED_WINDOW, 0)
{
	SetTitle(BPath(path).Leaf());

	drawview = new MediaView(Bounds(), "drawview", B_FOLLOW_ALL);
	AddChild(drawview);

	//drawview->SetColorSpace(B_CMAP8);
	drawview->SetMediaSource(path);
	
	float w = 0.0;
	float h = 0.0;
	drawview->GetPreferredSize(&w, &h);	

	SetSizeLimits(80.0, 30000.0, (drawview->HasVideoTrack()) ? 80.0 : h, (drawview->HasVideoTrack()) ? 30000.0 : h);
	SetZoomLimits(w, h);
	ResizeTo(w, h);

	drawview->MakeFocus(true);
	drawview->Control(MEDIA_PLAY);
}


DrawWindow::~DrawWindow()
{
	be_app->PostMessage(msg_WindowClosed);
}

void DrawWindow::MessageReceived(BMessage *message)
{
	switch (message->what)
	{
		default : BWindow::MessageReceived(message);
			break;
		case msg_RushDropped:
//			entry_ref	*ref;
			BPath		path;
			if (message->WasDropped())
			{
//				message->FindRef("ref", ref);
				if (message->FindFlat("ref", &path) == B_OK)
				{
					RemoveChild(drawview);
					delete drawview;
					drawview = new MediaView(Bounds(), "drawview", B_FOLLOW_ALL);
					AddChild(drawview);
					drawview->SetMediaSource(path.Path());
					
					float w = 0.0;
					float h = 0.0;
					drawview->GetPreferredSize(&w, &h);	
				
					SetSizeLimits(80.0, 30000.0, (drawview->HasVideoTrack()) ? 80.0 : h, (drawview->HasVideoTrack()) ? 30000.0 : h);
					SetZoomLimits(w, h);
					ResizeTo(w, h);
				
/*					drawview->MakeFocus(true);
					drawview->Control(MEDIA_PLAY);*/
				}
			}
			break;
	}
}