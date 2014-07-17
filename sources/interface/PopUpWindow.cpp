#include <InterfaceKit.h>
#include "PopUpWindow.h"
#include "MediaUtils.h"
#include "AllNodes.h"
#include "consts.h"


PopUpFiltre::PopUpFiltre(BRect frame) : BView(frame, "PopUpView", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE)
{
	BFont font;
	get_menu_info(&system);
	//font.SetFamilyAndStyle(system.f_family, system.f_style);
	font.SetFlags(B_DISABLE_ANTIALIASING);
	SetFont(&font);
	SetFontSize(10);
	BMessage *message = new BMessage(msg_time);
	Time = new BTextControl(BRect(5, 30, 170, 40), "begin time", "Time: ", "" , message, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	End = new BTextControl(BRect(5, 50, 170, 70), "End time", "Duration: ", "" , new BMessage(msg_end), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	Button = new BButton(BRect(5, 80, 50, 90), "OK", "OK", new BMessage(msg_ok), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	SetViewColor(system.background_color);
	AddChild(Button);
	AddChild(Time);
	AddChild(End);
	ResizeTo(175, 105);
}

PopUpFiltre::~PopUpFiltre()
{
}

void PopUpFiltre::Draw(BRect)
{
	BRect frame = Bounds();
	MovePenTo(BPoint(5, 15));
	DrawString("Filter: ");
	DrawString(((PopUpWin *)Window())->Save->name);
}

void PopUpFiltre::AttachedToWindow(void)
{
	BString string;
	string << ((PopUpWin *)Window())->Save->time;
	Time->SetText(string.String());
	string.SetTo("");
	string << ((PopUpWin *)Window())->Save->end;
	End->SetText(string.String());
}

PopUpVideo::PopUpVideo(BRect frame) : BView(frame, "PopUpView", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE)
{
	BFont font;
	get_menu_info(&system);
	font.SetFlags(B_DISABLE_ANTIALIASING);
	SetFont(&font);
	SetFontSize(10);
	Time = new BTextControl(BRect(5, 30, 170, 40), "time", "Time: ", "" , new BMessage(msg_time), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	Begin = new BTextControl(BRect(5, 50 , 170, 60), "Begin time", "Begin: ", "" , new BMessage(msg_begin), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	End = new BTextControl(BRect(5, 70, 170, 80), "End time", "Duration: ", "" , new BMessage(msg_end), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	Button = new BButton(BRect(5, 90, 50, 100), "OK", "OK", new BMessage(msg_ok), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	End->SetEnabled(false);
	SetViewColor(system.background_color);
	AddChild(Time);
	AddChild(Begin);
	AddChild(End);
	AddChild(Button);
	ResizeTo(250, 125);
}

PopUpVideo::~PopUpVideo()
{
}

void PopUpVideo::Draw(BRect)
{
	MovePenTo(BPoint(5, 15));
	DrawString("Video: ");
	DrawString(((PopUpWin *)Window())->Save->u.video.filepath);
	
}

void PopUpVideo::AttachedToWindow(void)
{
	BString string;
	string << ((((PopUpWin *)Window())->Save->time) / 1000);
	Time->SetText(string.String());
	string.SetTo("");
	string << ((((PopUpWin *)Window())->Save->u.video.begin) / 1000);
	Begin->SetText(string.String());
	string.SetTo("");
	string << ((((PopUpWin *)Window())->Save->end) / 1000);
	End->SetText(string.String());
}

PopUpTransition::PopUpTransition(BRect frame) : BView(frame, "PopUpView", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE)
{
	BFont font;
	get_menu_info(&system);
	font.SetFlags(B_DISABLE_ANTIALIASING);
	SetFont(&font);
	SetFontSize(10);
	BMessage *message = new BMessage(msg_time);
	Time = new BTextControl(BRect(5, 30, 170, 40), "begin time", "Time: ", "" , message, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	End = new BTextControl(BRect(5, 50, 170, 70), "End time", "Duration: ", "" , new BMessage(msg_end), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	Button = new BButton(BRect(5, 80, 50, 90), "OK", "OK", new BMessage(msg_ok), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	SetViewColor(system.background_color);
	AddChild(Button);
	AddChild(Time);
	AddChild(End);
	ResizeTo(175, 105);
}

PopUpTransition::~PopUpTransition()
{
}

void PopUpTransition::Draw(BRect)
{
	BRect frame = Bounds();
	MovePenTo(BPoint(5, 15));
	DrawString("Transition: ");
	DrawString(((PopUpWin *)Window())->Save->name);
}

void PopUpTransition::AttachedToWindow(void)
{
	BString string;
	string << ((PopUpWin *)Window())->Save->time;
	Time->SetText(string.String());
	string.SetTo("");
	string << ((PopUpWin *)Window())->Save->end;
	End->SetText(string.String());
}

PopUpWin::PopUpWin() : BWindow(BRect(0.0, 0.0, 0.0, 0.0), "Info!", B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS)
{
	Filtre = new PopUpFiltre(Bounds());
	Video = new PopUpVideo(Bounds());
	Transition = new PopUpTransition(Bounds());
	roster = BMediaRoster::Roster();
	messenger = new BMessenger(this);
	ParamView = NULL;
	nodeWatched = false;
}

PopUpWin::~PopUpWin()
{

}

void PopUpWin::Adjust(EventComposant *Composant)
{
	BParameterWeb	*web;
	BMediaTheme		*theme;
	
	Save = Composant;
	RemoveChild(Video);
	RemoveChild(Transition);
	RemoveChild(Filtre);
	if (ParamView)
		RemoveChild(ParamView);
	MoveTo(0,0);
	ResizeTo(0,0);
	switch (Save->event)
	{
		case filter:
			AddChild(Filtre);
			MoveTo(400,400);
			ResizeTo(175, 105);			
			InstantiateNode();
			roster->StartWatching(*messenger, node->Node(), B_MEDIA_NEW_PARAMETER_VALUE);
			nodeWatched = true;
			roster->GetParameterWebFor(node->Node(), &web);
			if (web)
			{
				theme = BMediaTheme::PreferredTheme();
				ParamView = BMediaTheme::ViewFor(web, 0, theme);
				if (Lock())
				{
					ParamView->MoveTo(0, Filtre->Bounds().bottom);
					ParamView->ResizeTo(Bounds().Width(), ParamView->Bounds().Height());
					ResizeBy(0, ParamView->Bounds().Height());
					AddChild(ParamView);
					Unlock();
				}
			}
			break;
		case video1:
		case video2:
			AddChild(Video);
			MoveTo(400,400);
			ResizeTo(250, 125);
			break;
		case transition:
			AddChild(Transition);
			MoveTo(400,400);
			ResizeTo(175, 105);
			InstantiateNode();
			roster->StartWatching(*messenger, node->Node(), B_MEDIA_NEW_PARAMETER_VALUE);
			nodeWatched = true;
			roster->GetParameterWebFor(node->Node(), &web);
			if (web)
			{
				theme = BMediaTheme::PreferredTheme();
				ParamView = BMediaTheme::ViewFor(web, 0, theme);
				if (Lock())
				{
					ParamView->MoveTo(0, Transition->Bounds().bottom);
					ParamView->ResizeTo(Bounds().Width(), ParamView->Bounds().Height());
					ResizeBy(0, ParamView->Bounds().Height());
					AddChild(ParamView);
					Unlock();
				}
			}
			break;
	}
}

void PopUpWin::MessageReceived(BMessage *message)
{
	BTextControl	*control;
	new BMessage(msg_PopUpDraw);
	const char	*bouh;
	char	*nathallo;
	char	boullay[16];
	
	switch (message->what)
	{
		case msg_time:
			message->FindPointer("source", ((void**)&control));
			bouh = control->Text();
			strcpy(boullay, bouh);
			nathallo = &boullay[0];
			nathallo += strlen(boullay);
			Save->time = strtoll(boullay, &nathallo, 10);
			be_app->PostMessage(msg_PopUpDraw);
			break;
		case msg_begin:
			message->FindPointer("source", ((void**)&control));
			bouh = control->Text();
			strcpy(boullay, bouh);
			nathallo = &boullay[0];
			nathallo += strlen(boullay);
			Save->u.video.begin = strtoll(boullay, &nathallo, 10);
			be_app->PostMessage(msg_PopUpDraw);
			break;
		case msg_end:
			message->FindPointer("source", ((void**)&control));
			bouh = control->Text();
			strcpy(boullay, bouh);
			nathallo = &boullay[0];
			nathallo += strlen(boullay);
			Save->end = strtoll(boullay, &nathallo, 10);
			be_app->PostMessage(msg_PopUpDraw);
			break;
		case msg_ok:
			if (nodeWatched)
			{
				roster->StopWatching(*messenger);
				roster->ReleaseNode(node->Node());
				node = NULL;
				nodeWatched = false;
			}
			Hide();
			break;
		case B_MEDIA_NEW_PARAMETER_VALUE:
			HandleMediaMessage(message);
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}

void PopUpWin::HandleMediaMessage(BMessage *message)
{
	BParameterWeb			*web;
	BParameterGroup			*group;
	BParameter				*param;
	parameter_list_elem		*elem;
	int32					id;
	void					*value;
	size_t					size;
	int 					i;
	bigtime_t				dummy;
	
	
//	message->FindInt32("parameter", &id);
//	message->FindData("value", B_RAW_TYPE, &value, &size);
	
	roster->GetParameterWebFor(node->Node(), &web);
	group = web->GroupAt(0);
	for (i = 0; i < group->CountParameters(); i++)
	{
		param = group->ParameterAt(i);
		elem = new parameter_list_elem;
		elem->id = param->ID();
		param->GetValue(&value, &size, &dummy);
		elem->value = malloc(size);
		memcpy(elem->value, &value, size);
		elem->value_size = size;
		if (Save->event == filter)
			Save->u.filter.param_list->AddItem(elem);
		else
			Save->u.transition.param_list->AddItem(elem);
	}
//	elem = new parameter_list_elem;
//	elem->id = id;
//	elem->value_size = size;
//	elem->value = malloc(size);
//	memcpy(elem->value, value, size);

}

void PopUpWin::InstantiateNode()
{
	if (Save->event == filter)
	{
		switch (Save->u.filter.type)
		{
			case blur:
				node = (BMediaNode*)(new BlurFilter);
				break;
			case black_white:
				node = (BMediaNode*)(new BWThresholdFilter);
				break;
			case contrast_brightness:
				node = (BMediaNode*)(new ContrastBrightnessFilter);
				break;
			case diff_detection:
				node = (BMediaNode*)(new DiffDetectionFilter);
				break;
			case emboss:
				node = (BMediaNode*)(new EmbossFilter);
				break;
			case frame_bin_op:
				node = (BMediaNode*)(new FrameBinOpFilter);
				break;
			case gray:
				node = (BMediaNode*)(new GrayFilter());
				break;
			case hv_mirror:
				node = (BMediaNode*)(new HVMirroringFilter);
				break;
			case invert:
				node = (BMediaNode*)(new InvertFilter);
				break;
			case levels:
				node = (BMediaNode*)(new LevelsFilter);
				break;
			case mix:
				node = (BMediaNode*)(new MixFilter);
				break;
			case motion_blur:
				node = (BMediaNode*)(new MotionBlurFilter);
				break;
			case motion_bw_threshold:
				node = (BMediaNode*)(new MotionBWThresholdFilter);
				break;
			case motion_rgb_threshold:
				node = (BMediaNode*)(new MotionRGBThresholdFilter);
				break;
			case motion_mask:
				node = (BMediaNode*)(new MotionMaskFilter);
				break;
			case mozaic:
				node = (BMediaNode*)(new MozaicFilter);
				break;
			case offset:
				node = (BMediaNode*)(new OffsetFilter);
				break;
			case rgb_intensity:
				node = (BMediaNode*)(new RGBIntensityFilter);
				break;
			case rgb_threshold:
				node = (BMediaNode*)(new RGBThresholdFilter);
				break;
			case solarize:
				node = (BMediaNode*)(new SolarizeFilter);
				break;
			case step_motion:
				node = (BMediaNode*)(new StepMotionBlurFilter);
				break;
			case trame:
				node = (BMediaNode*)(new TrameFilter);
				break;
			case rgb_channel:
				node = (BMediaNode*)(new ColorFilter);
				break;
		}
	}else if (Save->event == transition)
	{
		switch (Save->u.transition.type)
		{
			case cross_fader:
				node = (BMediaNode*)(new CrossFaderTransition);
				break;
			case disolve:
				node = (BMediaNode*)(new DisolveTransition);
				break;
			case flip:
				node = (BMediaNode*)(new FlipTransition);
				break;
			case gradient:
				node = (BMediaNode*)(new GradientTransition);
				break;
			case swap_transition:
				node = (BMediaNode*)(new SwapTransition);
				break;
			case venetian_stripes:
				node = (BMediaNode*)(new VenetianStripesTransition);
				break;
			case wipe:
				node = (BMediaNode*)(new WipeTransition);
				break;
		}
	}

}
