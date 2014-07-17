// MediaConverter.cpp
//
//   Copyright 1999, Be Incorporated.   All Rights Reserved.
//   This file may be used under the terms of the Be Sample Code License.

#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <Directory.h>
#include <Entry.h>
#include <ListItem.h>
#include <MediaFile.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <MediaTrack.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h> // for MAX() macro
#include <Path.h>
#include <Entry.h>

#include "ProjectPrefsWin.h"
#include "consts.h"

const char APP_SIGNATURE[]		= "application/x-vnd.Be.MediaConverter";
const char SOURCE_BOX_LABEL[]	= "Source files";
const char INFO_BOX_LABEL[]		= "File details";
const char OUTPUT_BOX_LABEL[]	= "Output format";
const char FORMAT_LABEL[] 		= "File format";
const char VIDEO_LABEL[] 		= "Video encoding";
const char AUDIO_LABEL[] 		= "Audio encoding";
const char FORMAT_MENU_LABEL[] 	= "Format";
const char VIDEO_MENU_LABEL[] 	= "Video";
const char AUDIO_MENU_LABEL[] 	= "Audio";
const char DURATION_LABEL[]		= "Duration:";
const char VIDEO_INFO_LABEL[]	= "Video:";
const char AUDIO_INFO_LABEL[]	= "Audio:";
const char SAVE_LABEL[]			= "Save";
const char CANCEL_LABEL[]		= "Cancel";

const uint32 CONVERT_BUTTON_MESSAGE		= 'cVTB';
const uint32 FORMAT_SELECT_MESSAGE		= 'fMTS';
const uint32 AUDIO_CODEC_SELECT_MESSAGE	= 'aCSL';
const uint32 VIDEO_CODEC_SELECT_MESSAGE	= 'vCSL';
const uint32 FILE_LIST_CHANGE_MESSAGE	= 'fLCH';
const uint32 CHOOSE_MESSAGE				= 'stCV';
const uint32 CHOSEN_MESSAGE				= 'cNCV';
const uint32 CONVERSION_DONE_MESSAGE	= 'cVSD';


// ------------------- FileFormatMenuItem -------------------

class FileFormatMenuItem : public BMenuItem
{
public:
				FileFormatMenuItem(media_file_format *format);
	virtual		~FileFormatMenuItem();
	
	media_file_format fFileFormat;
};

FileFormatMenuItem::FileFormatMenuItem(media_file_format *format)
	: BMenuItem(format->pretty_name, new BMessage(FORMAT_SELECT_MESSAGE))
{
	memcpy(&fFileFormat, format, sizeof(fFileFormat));
}


FileFormatMenuItem::~FileFormatMenuItem()
{
}

// ------------------- CodecMenuItem -------------------

class CodecMenuItem : public BMenuItem
{
public:
				CodecMenuItem(media_codec_info *ci, uint32 msg_type);
	virtual		~CodecMenuItem();
	
	media_codec_info fCodecInfo;
};

CodecMenuItem::CodecMenuItem(media_codec_info *ci, uint32 msg_type)
	: BMenuItem(ci->pretty_name, new BMessage(msg_type))
{
	memcpy(&fCodecInfo, ci, sizeof(fCodecInfo));
}


CodecMenuItem::~CodecMenuItem()
{
}


// ------------------- ProjectPrefsWin implementation -------------------


ProjectPrefsWin::ProjectPrefsWin(BRect frame)
	: BWindow(frame, "Project Settings", B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			  B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	fEnabled = true;
	fConverting = false;
	fCancelling = false;
	
	BRect r(frame);
	r.OffsetTo(0, 0);
	BView *background = new BView(r, NULL, B_FOLLOW_ALL_SIDES, 0);
	rgb_color c = ui_color(B_PANEL_BACKGROUND_COLOR);
	background->SetViewColor(c);
	background->SetLowColor(c);
	r.InsetBy(5, 5);

//	BRect r2(r);
//	r2.bottom -= 30;
//	r2.right = r2.left + 150;
//
//	BRect r3(r2);
//	r3.OffsetTo(0, 0);
//	r3.InsetBy(5, 5);
//	r3.top += 12;
////	r3.bottom -= B_H_SCROLL_BAR_HEIGHT;
//	r3.right -= B_V_SCROLL_BAR_WIDTH;
//	
//	r2.left = r2.right + 5;
//	r2.right = r.right - 5;
//	r2.bottom = r2.top + 120;
//	
//	r3 = r2;
//	r3.OffsetTo(0, 0);
//	r3.InsetBy(5, 5);
//	r3.top += 12;
//
//	r2.top = r2.bottom + 5;
//	r2.bottom = r.bottom - 30;
//	r2.OffsetTo(0, 0);
//	BBox *box = new BBox(r2, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM);
//	box->SetLabel(OUTPUT_BOX_LABEL);

	BRect r3 = r;
	r3.OffsetTo(0, 0);
	r3.InsetBy(5, 5);
	r3.top += 12;
	
	BRect r4(r3);
	r4.bottom = r4.top + 20;
	BPopUpMenu *popmenu = new BPopUpMenu(FORMAT_MENU_LABEL);
	fFormatMenu = new BMenuField(r4, NULL, FORMAT_LABEL, popmenu);
	float maxLabelLen = fFormatMenu->StringWidth(FORMAT_LABEL);
	background->AddChild(fFormatMenu);
	
	r4.top = r4.bottom + 5;
	r4.bottom = r4.top + 20;
	popmenu = new BPopUpMenu(AUDIO_MENU_LABEL);
	fAudioMenu = new BMenuField(r4, NULL, AUDIO_LABEL, popmenu);
	maxLabelLen = MAX(maxLabelLen, fAudioMenu->StringWidth(AUDIO_LABEL));
	background->AddChild(fAudioMenu);

	r4.top = r4.bottom + 5;
	r4.bottom = r4.top + 20;
	popmenu = new BPopUpMenu(VIDEO_MENU_LABEL);
	fVideoMenu = new BMenuField(r4, NULL, VIDEO_LABEL, popmenu);
	maxLabelLen = MAX(maxLabelLen, fVideoMenu->StringWidth(VIDEO_LABEL));
	background->AddChild(fVideoMenu);
//	background->AddChild(box);

	r4.top = r4.bottom + 10;
	r4.bottom = r4.top + 10;
	r4.right = r.right - 70;
	fileName = new BTextControl(r4, "File Name", "File Name:", "", NULL);
	fileName->SetDivider(fileName->StringWidth("File Name:") + 10);
	AddChild(fileName);
	
	r4.left = r4.right + 5;
	r4.right = r4.left + 60;
	fChooseButton = new BButton(r4, "ChooseButton", "Choose...", new BMessage(CHOOSE_MESSAGE));
	AddChild(fChooseButton);

	maxLabelLen += 5;
	fFormatMenu->SetDivider(maxLabelLen);
	fAudioMenu->SetDivider(maxLabelLen);
	fVideoMenu->SetDivider(maxLabelLen);

	BRect r30 = r;
	r30.InsetBy(5, 5);
	r30.left = r30.right - 30;
	r30.top = r30.bottom - 10;
	fConvertButton = new BButton(r30, NULL, SAVE_LABEL, new BMessage(CONVERT_BUTTON_MESSAGE));
	fConvertButton->MakeDefault(true);
	background->AddChild(fConvertButton);
	fConvertButton->ResizeToPreferred();
	BRect buttonFrame(fConvertButton->Frame());
	buttonFrame.OffsetTo(r.right - buttonFrame.Width(),
						 r.bottom - buttonFrame.Height());
	fConvertButton->MoveTo(buttonFrame.LeftTop());
//	r2.right = buttonFrame.left - 5;

	AddChild(background);
	BuildFormatMenu();
	
	fFilePanel = new BFilePanel(B_SAVE_PANEL, NULL, NULL, 0, false, new BMessage(CHOSEN_MESSAGE));
	fFilePanel->SetTarget(this);
}


ProjectPrefsWin::~ProjectPrefsWin()
{
}

void
ProjectPrefsWin::BuildAudioVideoMenus()
{
	BMenu *menu = fAudioMenu->Menu();
	BMenuItem *item;
	// clear out old audio codec menu items
	while ((item = menu->RemoveItem((int32)0)) != NULL) {
		delete item;
	}

	// get selected file format
	FileFormatMenuItem *ffmi = (FileFormatMenuItem*)fFormatMenu->Menu()->FindMarked();
	media_file_format *mf_format = &(ffmi->fFileFormat);

	media_format format, outfmt;
	memset(&format, 0, sizeof(format));
	media_codec_info codec_info;
	int32 cookie = 0;
	CodecMenuItem *cmi;

	// add available audio encoders to menu
	format.type = B_MEDIA_RAW_AUDIO;
	format.u.raw_audio = media_raw_audio_format::wildcard;	
	while (get_next_encoder(&cookie, mf_format, &format, &outfmt, &codec_info) == B_OK) {
		cmi = new CodecMenuItem(&codec_info, AUDIO_CODEC_SELECT_MESSAGE);
		menu->AddItem(cmi);
		// reset media format struct
		format.type = B_MEDIA_RAW_AUDIO;
		format.u.raw_audio = media_raw_audio_format::wildcard;
	}

	// mark first audio encoder
	item = menu->ItemAt(0);
	if (item != NULL) {
		fAudioMenu->SetEnabled(fEnabled);
		item->SetMarked(true);
		((BInvoker *)item)->Invoke();
	} else {
		item = new BMenuItem("None available", NULL);
		menu->AddItem(item);
		item->SetMarked(true);
		fAudioMenu->SetEnabled(false);
	}
	
	// clear out old video codec menu items
	menu = fVideoMenu->Menu();
	while ((item = menu->RemoveItem((int32)0)) != NULL) {
		delete item;
	}

	// construct a generic video format.  Some of these parameters
	// seem silly, but are needed for R4.5.x, which is more picky
	// than subsequent BeOS releases will be.
	memset(&format, 0, sizeof(format));
	format.type = B_MEDIA_RAW_VIDEO;
	format.u.raw_video.last_active = (uint32)(320 - 1);
	format.u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
	format.u.raw_video.display.format = B_RGB32;
	format.u.raw_video.display.line_width = (int32)320;
	format.u.raw_video.display.line_count = (int32)240;
	format.u.raw_video.display.bytes_per_row = 4 * 320;

	// add available video encoders to menu
	cookie = 0;
	while (get_next_encoder(&cookie, mf_format, &format, &outfmt, &codec_info) == B_OK) {
		cmi = new CodecMenuItem(&codec_info, VIDEO_CODEC_SELECT_MESSAGE);
		menu->AddItem(cmi);
	}

	// mark first video encoder
	item = menu->ItemAt(0);
	if (item != NULL) {
		fVideoMenu->SetEnabled(fEnabled);
		item->SetMarked(true);
		((BInvoker *)item)->Invoke();
	} else {
		item = new BMenuItem("None available", NULL);
		menu->AddItem(item);
		item->SetMarked(true);
		fVideoMenu->SetEnabled(false);
	}
}

void
ProjectPrefsWin::GetSelectedFormatInfo(media_file_format *format,
											media_codec_info *audio,
											media_codec_info *video)
{
	FileFormatMenuItem *formatItem =
		dynamic_cast<FileFormatMenuItem *>(fFormatMenu->Menu()->FindMarked());
	if (formatItem != NULL) {
		*format = formatItem->fFileFormat;
	}
	
	CodecMenuItem *codecItem =
		dynamic_cast<CodecMenuItem *>(fAudioMenu->Menu()->FindMarked());
	if (codecItem != NULL) {
		*audio =  (codecItem->fCodecInfo);
	}
	codecItem = dynamic_cast<CodecMenuItem *>(fVideoMenu->Menu()->FindMarked());
	if (codecItem != NULL) {
		*video =  (codecItem->fCodecInfo);
	}
}

void ProjectPrefsWin::GetSelectedEntry(entry_ref *ref)
{
	*ref = theFile;
}

void 
ProjectPrefsWin::BuildFormatMenu()
{
	BMenu *menu = fFormatMenu->Menu();
	BMenuItem *item;
	// clear out old format menu items
	while ((item = menu->RemoveItem((int32)0)) != NULL) {
		delete item;
	}

	// add menu items for each file format
	media_file_format mfi;
	int32 cookie = 0;
	FileFormatMenuItem *ff_item;
	while (get_next_file_format(&cookie, &mfi) == B_OK)
	{
		if ((mfi.capabilities & media_file_format::B_KNOWS_ENCODED_VIDEO) || (mfi.capabilities & media_file_format::B_KNOWS_RAW_VIDEO))
		{
			ff_item = new FileFormatMenuItem(&mfi);
			menu->AddItem(ff_item);
		}
	}
	
	// mark first item
	item = menu->ItemAt(0);
	if (item != NULL) {
		item->SetMarked(true);
		((BInvoker *)item)->Invoke();
	}
}

void 
ProjectPrefsWin::SetEnabled(bool enabled, bool buttonEnabled)
{
	fConvertButton->SetEnabled(buttonEnabled);
	if (enabled != fEnabled) {
		fFormatMenu->SetEnabled(enabled);
		fAudioMenu->SetEnabled(enabled);
		fVideoMenu->SetEnabled(enabled);
		fEnabled = enabled;
	}
}

bool 
ProjectPrefsWin::IsEnabled()
{
	return fEnabled;
}

void 
ProjectPrefsWin::DispatchMessage(BMessage *msg, BHandler *handler)
{
	if (msg->WasDropped() && msg->what == B_SIMPLE_DATA) {
		DetachCurrentMessage();
		msg->what = B_REFS_RECEIVED;
		be_app->PostMessage(msg);
		delete msg;
	} else {
		BWindow::DispatchMessage(msg, handler);
	}
}


void 
ProjectPrefsWin::MessageReceived(BMessage *msg)
{
	BPath		path;
	BDirectory	dir;
	entry_ref	dirRef;
	const char		*bouh;
	switch (msg->what) {
	case FORMAT_SELECT_MESSAGE:
		BuildAudioVideoMenus();
		break;
	case AUDIO_CODEC_SELECT_MESSAGE:
		break;
	case VIDEO_CODEC_SELECT_MESSAGE:
		break;
	case CONVERT_BUTTON_MESSAGE:
		bouh = fileName->Text();
		entry.SetTo(bouh);
		entry.GetRef(&theFile);
		be_app->PostMessage(new BMessage(MEDIA_FORMATS));
		Hide();
		break;
	case CHOOSE_MESSAGE:
		fFilePanel->Show();
		break;
	case CHOSEN_MESSAGE:
		msg->FindRef("directory", &dirRef);
		msg->FindString("name", &bouh);
		dir.SetTo(&dirRef);
		entry.SetTo(&dir, bouh);
		entry.GetRef(&theFile);
		entry.SetTo(&theFile);
		entry.GetPath(&path);
		fileName->SetText(path.Path());
		break;
	default:
		BWindow::MessageReceived(msg);
	}
}

bool
ProjectPrefsWin::QuitRequested()
{
	return true;
}

/*
void 
MediaConverterApp::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case FILE_LIST_CHANGE_MESSAGE:
		if (fWin->Lock()) {
			bool enable = fWin->CountSourceFiles() > 0;
			fWin->SetEnabled(enable, enable);
			fWin->Unlock();
		}
		break;
	case START_CONVERSION_MESSAGE:
		if (!fConverting) {
			StartConverting();			
		}
		break;
	case CANCEL_CONVERSION_MESSAGE:
		fCancel = true;
		break;
	case CONVERSION_DONE_MESSAGE:
		fCancel = false;
		fConverting = false;
		DetachCurrentMessage();
		fWin->PostMessage(msg);
		break;
	default:
		BApplication::MessageReceived(msg);
	}
}*/