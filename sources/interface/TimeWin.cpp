#include "TimeWin.h"
#include "consts.h"
#include <Screen.h>
#include <ScrollView.h>
#include <String.h>
#include <Slider.h>
#include "EventList.h"
#include <Font.h>
#include <Alert.h>

  /*******************************************************************************/
 /*********************    TimeView  ********************************************/
/*******************************************************************************/

TimeView::TimeView(BRect frame) : BView(frame, "TimeView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE)
{
	film_duration = 1000000000;
	scale = 25;
	VideoRect1 = new BRect(frame);
	VideoRect1->top = 30;
	VideoRect1->bottom = 60;
	VideoRect1->left = 0;
	TransitionRect = new BRect(frame);
	TransitionRect->top = 60;
	TransitionRect->bottom = 80;
	TransitionRect->left = 0;
	VideoRect2 = new BRect(frame);
	VideoRect2->top = 80;
	VideoRect2->bottom = 110;
	VideoRect2->left = 0;
	List = new EventList;
	shift = false;
	before.x = 0;
}

TimeView::~TimeView()
{
	delete List;
	delete 	VideoRect1;
	delete 	VideoRect2;
}

void TimeView::Draw(BRect updateRect)
{
	rgb_color	oldColor; 
	rgb_color	scribColor;
	SetHighColor(black);
	int	start, end;
	int i;
	start = (int(updateRect.left / scale) ) * scale;
	end = (int(updateRect.right / scale) + 1) * scale;
	int nb = (end - start) / scale * 2 + 3;
	if (nb > 0) // Le rectangle peut etre trop petit pour avoir a dessiner kkchose
				// dans ce cas, nb est <0, donc on ne dessine rien
	{
		BeginLineArray(nb);
		for (i=start; i <= end; i += scale)
		{
			AddLine(BPoint(i, 0), BPoint(i, 10), black);
			AddLine(BPoint(i + scale / 2, 0), BPoint(i + scale / 2, 5), black);
		}
		EndLineArray();
		for (i=start; i <= end; i += scale)
		{
			BString str;
			int		sec = i / scale;
			if (!(sec % 5))
			{
				str << uint32(sec / 60);
				str << ":";
				str << uint32(sec % 60);
				MovePenTo(i - StringWidth(str.String())/2, 20);
				DrawString(str.String());
			}
		}
		BeginLineArray(4);
		AddLine(BPoint(updateRect.left, 30), BPoint(updateRect.right, 30), black);
		AddLine(BPoint(updateRect.left, 60), BPoint(updateRect.right, 60), black);
		AddLine(BPoint(updateRect.left, 80), BPoint(updateRect.right, 80), black);
		AddLine(BPoint(updateRect.left, 110), BPoint(updateRect.right, 110), black);
		EndLineArray();
	}
	
	
	if (List->CountItems() != 0)
	{
		BRect	r;
		oldColor = HighColor();
		int top = 0, bottom = 0;
		
		for (i = 0; i < List->CountItems(); i++) //on dessine les differents Rect de la liste.
		{
			if (List->ItemAt(i)->event == video1)
			{
				if (List->ItemAt(i)->select == false)
				{
					SetHighColor(hiliteColor);
					scribColor = yellow;
				}
				else
				{
					SetHighColor(greenSelect);
					scribColor = black;
				}
				top = 30;
				bottom = 60;
			}
			if (List->ItemAt(i)->event == video2)
			{
				if (List->ItemAt(i)->select == false)
				{
					SetHighColor(hiliteColor);
					scribColor = yellow;
				}
				else
				{
					SetHighColor(greenSelect);
					scribColor = black;
				}
				top = 80;
				bottom = 110;
			}
			if ((List->ItemAt(i)->event == transition))
			{
				if (List->ItemAt(i)->select == false)
				{
					SetHighColor(boullayColor);
					scribColor = yellow;
				}
				else
				{	
					SetHighColor(greenSelect);
					scribColor = black;
				}
				top = 60;
				bottom = 80;
			}
			if ((List->ItemAt(i)->event == filter))
			{
				if (List->ItemAt(i)->select == false)
				{
					SetHighColor(smoothColor);
					scribColor = yellow;
				}
				else
				{	
					SetHighColor(greenSelect);
					scribColor = black;
				}
				top = 60;
				bottom = 80;
			}
				
			r.top = top;
			r.bottom = bottom;
			r.left = ((List->ItemAt(i)->time) * scale) / 1000000;
			r.right = ((((List->ItemAt(i)->time + List->ItemAt(i)->end)) * scale) / 1000000);
			
			FillRect(r);
			SetHighColor(black);
			StrokeRect(r);
			SetHighColor(scribColor);
			BFont font;
			GetFont(&font);
			font.SetSize(10.0);
			SetFont(&font);
			font.SetFlags(B_DISABLE_ANTIALIASING);
			font.SetEncoding(B_TRUETYPE_WINDOWS);
			font.SetFace(B_REGULAR_FACE);
			font.SetSpacing(B_CHAR_SPACING);
			BString string;
			if (List->ItemAt(i)->event == video1 || List->ItemAt(i)->event == video2)
				string << ((List->ItemAt(i)->u.video.filepath)) << " (" << (float)((List->ItemAt(i)->end) / (float)1000000) << " sec)";
			else
				string << List->ItemAt(i)->name << " (" << (float)((List->ItemAt(i)->end) / (float)1000000) << " sec)";
			MovePenTo(((r.left + r.right) / 2 - (font.StringWidth(string.String()) /2)), (r.top + r.bottom) / 2 + font.Size() / 2);
			SetFont(&font);
			DrawString(string.String());
			
			SetHighColor(oldColor);
		}
	}
}


void TimeView::MouseDown(BPoint point)
{
	BPoint	where = point;
	before.x = where.x;
	shift = modifiers() & B_SHIFT_KEY;
	in = false;
	resized = false;
	move = false;
	
	BRect	r;
	int top = 0, bottom = 0;
	BMessage* msg = Window()->CurrentMessage();
	int32 clicks = msg->FindInt32("clicks");
	int32 button = msg->FindInt32("buttons");
		
	for (int i = 0; i < List->CountItems(); i++)
	{
		if (List->ItemAt(i)->event == video1)
		{
			top = 30;
			bottom = 60;
		}
		if (List->ItemAt(i)->event == video2)
		{
			top = 80;
			bottom = 110;
		}
		if ((List->ItemAt(i)->event == transition) || (List->ItemAt(i)->event == filter))
		{
			top = 60;
			bottom = 80;
		}
			
		r.top = top;
		r.bottom = bottom;
		r.left = ((List->ItemAt(i)->time) * scale) / 1000000;
		r.right = (((List->ItemAt(i)->time + List->ItemAt(i)->end) * scale) / 1000000);
			
		if ((r.Contains(where) == true)) //si le pointeur est ds le rectangle.
		{	
			in = true;
			switch (clicks)
			{
				case 1: //simple click
					in = true;
					if ((where.x >= ((((List->ItemAt(i)->time + List->ItemAt(i)->end) * scale) / 1000000) - 5)) &&
			 		(where.x <= (((List->ItemAt(i)->time + List->ItemAt(i)->end) * scale) / 1000000)))
						resized = true;
					else	
						move = true;
					List->ItemAt(i)->select = true;
					Invalidate();
				break;
				case 2: //double click
					Pop = new BMessage(msg_PopUp);
					Pop->AddPointer("Composant", List->ItemAt(i));
					be_app->PostMessage(Pop);
				break;
			}
		}
	}				 
				 
	if ((shift == false)) //si la touche shift n'est pas enfoncee.
	{
		for (int i = 0; i < List->CountItems(); i++) //on replace tous les selects a false.
		{
			List->ItemAt(i)->select = false;
		}
	}


	for (int i = 0; i < List->CountItems(); i++) //on dessine les differents Rect de la liste.
	{
		if (List->ItemAt(i)->event == video1)
		{
			top = 30;
			bottom = 60;
		}
		if (List->ItemAt(i)->event == video2)
		{
			top = 80;
			bottom = 110;
		}
		if ((List->ItemAt(i)->event == transition) || (List->ItemAt(i)->event == filter))
		{
			top = 60;
			bottom = 80;
		}
			
		r.top = top;
		r.bottom = bottom;
		r.left = ((List->ItemAt(i)->time) * scale) / 1000000;
		r.right = (((List->ItemAt(i)->time + List->ItemAt(i)->end) * scale) / 1000000);
		
		if (r.Contains(where) == true) //si le pointeur est ds le rectangle.
		{
			in = true;
			if (clicks == 1 || clicks == 2)
			{
				List->ItemAt(i)->select = true;
				Invalidate();
			}
		}
	}
	
}

void TimeView::MouseUp(BPoint point)
{
	BMessage* msg = Window()->CurrentMessage();
	int32 clicks = msg->FindInt32("clicks");
	int32 button = msg->FindInt32("buttons");
	clicks = 0;

	if (in == false)
	{
		for (int i = 0; i < List->CountItems(); i++) //on replace tous les selects a false.
		{
			List->ItemAt(i)->select = false;
			Invalidate();
		}
	}
	
	in = false;
	resized = false;
	move = false;
}

void TimeView::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	BPoint	where = point;
	
	if (resized == true)
	{
		for (int i = 0; i < List->CountItems(); i++)
		{
			if (List->ItemAt(i)->select == true)
			{
				if (List->ItemAt(i)->time >= (List->ItemAt(i)->time + List->ItemAt(i)->end) && where.x <= before.x)
				{
				}
				else
				
				{
					List->ItemAt(i)->end += ((where.x - before.x) * 1000000) / scale;
					Invalidate();
				}
				
			}
		}
	}

	if (move == true)
	{
		for (int i = 0; i < List->CountItems(); i++)
		{
			if (List->ItemAt(i)->select == true)
			{
				if ((List->ItemAt(i)->time >= 0) || (where.x > before.x)) // pr ne pas sortir de la timeline box par la gauche.
				{
					List->ItemAt(i)->time += ((where.x - before.x) / scale) * 1000000 ;
					Invalidate();
				}
			}
		}
	}
	before.x = where.x;
			
	if (message)
	{
		rgb_color	oldcolor;
		
		switch (message->what)
		{
			case msg_RushDropped:
				if (VideoRect1->Contains(point))
				{
					oldcolor = HighColor();
					SetHighColor(hiliteColor);
					FillRect(*VideoRect1);
					SetHighColor(oldcolor);
				}
	
				else if (VideoRect2->Contains(point))
				{	
					oldcolor = HighColor();
					SetHighColor(hiliteColor);
					FillRect(*VideoRect2);
					SetHighColor(oldcolor);
				}
				else Invalidate();
		
				if (transit == B_EXITED_VIEW)
					Invalidate();
				break;
				
			case msg_FxDropped:
				if (TransitionRect->Contains(point))
				{
					oldcolor = HighColor();
					SetHighColor(smoothColor);
					FillRect(*TransitionRect);
					SetHighColor(oldcolor);
				}
				
				else Invalidate();
				
				if (transit == B_EXITED_VIEW)
					Invalidate();
				break;
				
			case msg_TransitDropped:
				if (TransitionRect->Contains(point))
				{
					oldcolor = HighColor();
					SetHighColor(smoothColor);
					FillRect(*TransitionRect);
					SetHighColor(oldcolor);
				}
				
				else Invalidate();
				
				if (transit == B_EXITED_VIEW)
					Invalidate();
				break;
		}
	}
	
}

void TimeView::KeyDown(const char *bytes, int32 numBytes)
{
	if (bytes[0] == B_DELETE )
	{
		for (int i = 0; i < List->CountItems(); i++)
		{
			if (List->ItemAt(i)->select)
			{
				List->RemoveItem(i);
				Invalidate();
			}
		}
	}
}


void TimeView::FixupScrollBars()
{
	BRect bounds;
	BScrollBar *sb;

	bounds = Bounds();
	float myPixelWidth = bounds.Width();
	float myPixelHeight = bounds.Height();
	float maxWidth, maxHeight;

	maxWidth = scale * film_duration / 1000000;
	maxHeight = TIMEWIN_HEIGHT - B_H_SCROLL_BAR_HEIGHT;
			
	float propW = myPixelWidth/maxWidth;
	float propH = myPixelHeight/maxHeight;
	
	float rangeW = maxWidth - myPixelWidth;
	float rangeH = maxHeight - myPixelHeight;
	if(rangeW < 0) rangeW = 0;
	if(rangeH < 0) rangeH = 0;


	if ((sb=ScrollBar(B_HORIZONTAL))!=NULL) {
		sb->SetProportion(propW);
		sb->SetRange(0,rangeW);
		sb->SetSteps(myPixelWidth / 8.0, myPixelWidth / 1.2);
	} 

	if ((sb=ScrollBar(B_VERTICAL))!=NULL) {
		sb->SetProportion(propH);
		sb->SetRange(0,rangeH);
		// Steps are 1/8 visible window for small steps
		//   and 1/2 visible window for large steps
		sb->SetSteps(myPixelHeight / 8.0, myPixelHeight / 2.0);
	}	
}

void TimeView::AttachedToWindow(void)
{
	FixupScrollBars();
	MakeFocus();
}

void TimeView::FrameResized(float width, float height)
{
	FixupScrollBars();
}

void TimeView::MediaReceived(BMediaFile *media)
{

	film_duration = 1000000000;
	Window()->SetSizeLimits(20.0, scale * film_duration / 1000000, 20.0, TIMEWIN_HEIGHT);
	FixupScrollBars();
}

void TimeView::MessageReceived(BMessage *message)
{
	rgb_color	oldcolor;
	BEntry		entry;
	entry_ref	ref;
	BPath		path;
	switch (message->what)
	{
		default:
			BView::MessageReceived(message);
			break;
		case msg_RushDropped:
			if (message->WasDropped()) //qd on relache la souris
			{
				oldcolor = HighColor();
				BPoint	where = message->DropPoint();
				ConvertFromScreen(&where);
				Composant = new EventComposant;
				Composant->select = false;
				if (where.y < 60)
					Composant->event = video1;
				else
					Composant->event = video2;
				Composant->u.video.begin = 0;
				Composant->end = message->FindInt32("film_width"); //taille du film importe de la rush.
				message->FindRef("ref", &ref);
				entry.SetTo(&ref);
				entry.GetPath(&path);
				Composant->u.video.filepath = (char*) malloc(strlen(path.Path())+1);
				strcpy(Composant->u.video.filepath, path.Path());
				Composant->time = ((bigtime_t)((where.x  * 1000000)/ scale));
				List->AddItem(Composant);

				Invalidate();
			}
			break;
				
		case msg_FxDropped:
			if (message->WasDropped())
			{
				const char *name;
				BPoint where = message->DropPoint();
				ConvertFromScreen(&where);
				Composant = new EventComposant;
				Composant->select = false;
				Composant->event = filter;
				Composant->end = message->FindInt32("filter_width");
				Composant->u.filter.type = (filter_type)message->FindInt32("filter_type");
				message->FindString("ref", &name);
				Composant->name = (char*)malloc(strlen(name)+1);
				strcpy(Composant->name, name);
				Composant->time = ((bigtime_t)((where.x * 1000000) / scale));
				Composant->u.filter.param_list = new parameter_list;
				List->AddItem(Composant);

				Invalidate();
			}
			break;
			
		case msg_TransitDropped:
			if (message->WasDropped())
			{
				const char *name;
				BPoint where = message->DropPoint();
				ConvertFromScreen(&where);
				Composant = new EventComposant;
				Composant->select = false;
				Composant->event = transition;
				Composant->end = message->FindInt32("transition_width");
				Composant->u.transition.type = (transition_type)message->FindInt32("transition_type");
				message->FindString("ref", &name);
				Composant->name = (char*)malloc(strlen(name)+1);
				strcpy(Composant->name, name);
				Composant->time = ((bigtime_t)((where.x * 1000000) / scale));
				Composant->u.transition.param_list = new parameter_list;
				List->AddItem(Composant);

				Invalidate();
			}
			break;
			
	}
}

void TimeView::Rescale()
{
	TimeControlView *timeCtrl = (TimeControlView*)Window()->FindView("TimeControlView");
	//if (timeCtrl)
		scale = timeCtrl->scaleSlider->Value();
	Invalidate();
}

  /******************************************************************************/
 /*********************    TimeControlView  ************************************/
/******************************************************************************/

TimeControlView::TimeControlView(BRect frame) : BView(frame, "TimeControlView", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS)
{
	BRect	r = Bounds();
	r.bottom -= B_H_SCROLL_BAR_HEIGHT;
	r.InsetBy(25, 10);
	r.top = r.bottom - 15;
	scaleSlider = new BSlider(r, "Scale Slider", "Zoom out/in", new BMessage(msg_NewScale), 10, 100, B_TRIANGLE_THUMB);
	scaleSlider->SetModificationMessage(new BMessage(msg_NewScale));
	AddChild(scaleSlider);
	SetViewColor(background_color);
}

TimeControlView::~TimeControlView()
{
	
}

void TimeControlView::Draw(BRect updateRect)
{
	float	start, end;
	rgb_color	black = {0,0,0};
	start = updateRect.left;
	end = updateRect.right;
	
	BeginLineArray(4);
	AddLine(BPoint(start, 30), BPoint(end, 30), black);
	AddLine(BPoint(start, 60), BPoint(end, 60), black);
	AddLine(BPoint(start, 80), BPoint(end, 80), black);
	AddLine(BPoint(start, 110), BPoint(end, 110), black);
	EndLineArray();
	
	MovePenTo(CONTROLVIEW_WIDTH / 2 - StringWidth("Video1"), 50);
	DrawString("Video 1:");
	
	MovePenTo(CONTROLVIEW_WIDTH / 2 - StringWidth("Transition"), 75);
	DrawString("Transition:");
	MovePenTo(CONTROLVIEW_WIDTH / 2 - StringWidth("Video2"), 100);
	DrawString("Video 2:");

}

void TimeControlView::AttachedToWindow()
{
	scaleSlider->SetTarget(Parent());
	scaleSlider->SetValue(25);
}


  /******************************************************************************/
 /*********************    TimeWin  ********************************************/
/******************************************************************************/

TimeWin::TimeWin() : BWindow(BRect(0.0, 0.0, 0.0, 0.0), "TimeLinE BoX", B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_CLOSABLE | B_ASYNCHRONOUS_CONTROLS)
{
	BScreen		*theScreen = new BScreen(this);
	BRect		r = theScreen->Frame();
	
	//fenetre
	SetZoomLimits(r.right - 5, r.bottom / 2);
	r.InsetBy(50, 50);
	r.top = r.bottom - TIMEWIN_HEIGHT;
	ResizeTo(r.right - r.left, r.bottom - r.top);
	MoveTo(r.left, r.top);
	
	//vues...
	r.OffsetTo(0.0, 0.0); //on se place maintenant par rapport a la fenetre et plus l'ecran.
	r.right = CONTROLVIEW_WIDTH;
	tControl = new TimeControlView(r);
	AddChild(tControl);

	r.left = r.right + 1;
	r.right = Bounds().right;
	r.bottom -= B_H_SCROLL_BAR_HEIGHT;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	
	tView = new TimeView(r);
	AddChild(new BScrollView("Main View", tView, B_FOLLOW_ALL_SIDES, B_WILL_DRAW, true, true) );
	SetSizeLimits(20.0, theScreen->Frame().right, 20.0, TIMEWIN_HEIGHT);
	delete theScreen;
		
	tView->MediaReceived(NULL); // A retirer, juste pour tests
}

TimeWin::~TimeWin()
{

}
	
void TimeWin::MessageReceived(BMessage *message)
{
	switch (message->what)
	{
		case msg_NewScale:
			tView->Rescale();
			break;
		case msg_RushDropped:
//			tView->RushDropped(message);
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
	