#include "rushbox.h"
#include <Path.h>
#include <Message.h>
#include "consts.h"
#include <ScrollView.h>
#include <StorageKit.h>
#include <MediaFile.h>
#include <Alert.h>
#include <TranslationUtils.h>
#include "MediaUtils.h"

void rushwin::MessageReceived(BMessage *message)
{
	entry_ref *FilePath;
	RushListItem	*item;
	switch(message->what)
	{
		default: //si un msg inconnu arrive
			BWindow::MessageReceived(message); //BeOS s'en charge...
			break;
		case B_SIMPLE_DATA:
		case msg_ImportRush:
			FilePath = new entry_ref;
			message->FindRef("refs", FilePath);  //on extrait le BPath du fichier importé
			item = new RushListItem(FilePath);
			if (!item->InitCheck())
			{
				RushView->AddItem(item); //on recupere le nom seul du fichier que l'on place ds la liste a afficher
			}
			else
				(new BAlert("Bad Media", "This is not a Media File, or the codec for this media is not available", "Sorry"))->Go();
			break;
	}
}

rushwin::rushwin() : BWindow(BRect(5,120,RUSHBOX_WIDTH + 5, RUSHBOX_HEIGHT + 120), "Rush BoX", B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_CLOSABLE | B_NOT_H_RESIZABLE | B_ASYNCHRONOUS_CONTROLS)
{
	RushView = new RushListView(Bounds(),"List");
	AddChild(new BScrollView("scroll_rushes", RushView, B_FOLLOW_ALL_SIDES, B_WILL_DRAW, false, true));
}

rushwin::~rushwin()
{

}

RushListView::RushListView(BRect frame, char *name) : BListView(frame, name, B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS)
{

}

RushListView::~RushListView()
{

}

void RushListView::FixupScrollBars()
{
	BScrollBar *sb;
	BRect bounds;
	bounds = Bounds();
	float myPixelHeight = bounds.Height();
	float maxHeight = RUSHBOX_HEIGHT - B_H_SCROLL_BAR_HEIGHT;
	float propH = myPixelHeight/maxHeight;
	float rangeH = maxHeight - myPixelHeight;
	
	if ((sb=ScrollBar(B_VERTICAL))!=NULL) {
		sb->SetProportion(propH);
		sb->SetRange(0,rangeH);
		// Steps are 1/8 visible window for small steps
		//   and 1/2 visible window for large steps
		sb->SetSteps(36, 78);
	}
}

void RushListView::FrameResized(float width, float height)
{
	FixupScrollBars();
}

bool RushListView::InitiateDrag(BPoint point, int32 index, bool wasSelected)
{
	bool	result = wasSelected;
	if ( result )
	{
		BMessage message(msg_RushDropped);
		bigtime_t	time;
		status_t	error;
		BRect	selectRect = Bounds();
		float	itemSize = ItemAt(index)->Height();
		selectRect.top = index * itemSize;
		selectRect.bottom = selectRect.top + itemSize;
		entry_ref *ref = ((RushListItem*)ItemAt(index))->media_path;
		error = MediaDuration(*ref, &time);
		if (error != B_OK)
			(new BAlert("Alert !!!!!!", "This is not a good Media File, or the codec for this media is not available", "Sorry"))->Go();
		else
		{
			message.AddInt32("film_width", time);
			message.AddRef("ref", (entry_ref*)ref);
			DragMessage(&message, selectRect);
		}		
	}
	return result;

}

void RushListView::AttachedToWindow(void)
{
	FixupScrollBars();
	parent = (rushwin*) Window();
	MakeFocus();
}

void RushListView::KeyDown(const char *bytes, int32 numBytes)
{
	switch (bytes[0]) {
	case B_DELETE:
		 {
			int32 selection = CurrentSelection();
			if (selection >= 0) 
			{
				BListItem *item = RemoveItem(selection);
				if (item != NULL) {
					delete item;
				}
				int32 count = CountItems();
				if (selection >= count) {
					selection = count - 1;
				}
				Select(selection);
			}		
		}
		break;
	default:
		BListView::KeyDown(bytes, numBytes);
	}
}

RushListItem::RushListItem(entry_ref *filepath) : BListItem()
{
	media_path = new entry_ref(*filepath);
	preview = NULL;
}

RushListItem::~RushListItem()
{
	delete media_path;
	if (preview != NULL)
		delete preview;
}

void RushListItem::DrawItem(BView *owner, BRect frame, bool complete = false)
{
	rgb_color	color;
	if (IsSelected() || complete)
	{
		if (IsSelected())
			color = kHighlightColor;
		else
			color = owner->ViewColor();
		owner->SetHighColor(color);
		owner->FillRect(frame);
	}
	BRect	previewRect(frame.left + 2, frame.top + 2, frame.left + 42, frame.bottom - 3);
	owner->DrawBitmapAsync(preview, previewRect);
	owner->MovePenTo(previewRect.right + 5, (frame.top + frame.bottom) / 2);
	owner->SetHighColor(black); //on dessine en noir
	owner->StrokeRect(frame); //on dessine le rectangle autour de la preview
	owner->DrawString(media_path->name);
}

status_t RushListItem::InitCheck()
{
	status_t	err;
	file_type  what;
	if ((what = ReadFirstPicture(media_path, &preview)) != VIDEO_FILE)
	{ // il n'y a pas de video dans le fichier --> audio ?
		if (what == AUDIO_FILE)
		{
			//Workaround for the media server recognizing pictures as MEDIA_ENCODED_AUDIO ...
			BEntry	entry(media_path, true);
			BPath	path;
			if (entry.InitCheck() == B_OK)
			{
				entry.GetPath(&path);
				if ((preview = BTranslationUtils::GetBitmapFile(path.Path())) == NULL)
				{
					preview = BTranslationUtils::GetBitmap("AudioBitmap");
					err = B_OK;
				}
				else err = B_OK;
			}
			else err = B_ERROR;
		}
		else
		{
			BEntry	entry(media_path, true);
			BPath	path;
			if (entry.InitCheck() == B_OK)
			{
				entry.GetPath(&path);
				if ((preview = BTranslationUtils::GetBitmapFile(path.Path())) == NULL)
					err = B_ERROR;
				else err = B_OK;
			}
			else err = B_ERROR;
		}
	}
	else err = B_OK;
	return (err);
}

void RushListItem::Update(BView *owner, const BFont *font)
{
	BListItem::Update(owner, font);
	SetHeight(height);
}


