#include "TransitionBox.h"
#include <Path.h>
#include <Message.h>
#include "consts.h"
#include <ScrollView.h>
#include <StorageKit.h>
#include <MediaFile.h>
#include <Alert.h>
#include <TranslationUtils.h>
#include "MediaUtils.h"
#include <Screen.h>


Transitwin::Transitwin() : BWindow(BRect(0.0, 0.0, 0.0, 0.0), "TransitioN BoX", B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_CLOSABLE | B_NOT_H_RESIZABLE | B_ASYNCHRONOUS_CONTROLS)
{
	//on prend la resolution de l'ecran afin de bien placer les fenetres.
	BScreen		*theScreen = new BScreen(this);
	BRect		r = theScreen->Frame();
	
	//fenetre...
	SetZoomLimits(TRANSITIONBOX_WIDTH, TRANSITIONBOX_HEIGHT);
	r.InsetBy(5,5);
	ResizeTo(TRANSITIONBOX_WIDTH, TRANSITIONBOX_HEIGHT);
	MoveTo(r.right - (TRANSITIONBOX_WIDTH + 5), r.top + TRANSITIONBOX_HEIGHT + 45);

	//vues...
//	r.left = r.right + 1;
//	r.right = Bounds().right;
	r= Bounds();
//	r.bottom -= B_H_SCROLL_BAR_HEIGHT;
	r.right -= B_V_SCROLL_BAR_WIDTH;

	BView *owner;
	TransitView = new TransitListView(r,"List");
	AddChild(new BScrollView("Main View", TransitView, B_FOLLOW_ALL_SIDES, B_WILL_DRAW, false, true));
	
	}

Transitwin::~Transitwin()
{

}

TransitListView::TransitListView(BRect frame, char *name) : BListView(frame, name, B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS)
{
	
}

TransitListView::~TransitListView()
{

}

bool TransitListView::InitiateDrag(BPoint point, int32 index, bool wasSelected)
{
	bool	result = wasSelected;
	if ( result )
	{
		BMessage message(msg_TransitDropped);
		BRect	selectRect = Bounds();
		float	itemSize = ItemAt(index)->Height();
		selectRect.top = index * itemSize;
		selectRect.bottom = selectRect.top + itemSize;
		char *ref = ((TransitListItem*)ItemAt(index))->transition_name;
		message.AddInt32("transition_width", 10000000);
		message.AddInt32("transition_type", ((TransitListItem*)ItemAt(index))->type);
		message.AddString("ref", (char *)ref);
		DragMessage(&message, selectRect);		
	}
	return result;

}

void TransitListView::AttachedToWindow(void)
{
	FixupScrollBars();
	parent = (Transitwin*) Window();
	// initialisation des Transitions primaires.
	AddItem(new TransitListItem("Cross Fader", cross_fader));
	AddItem(new TransitListItem("Disolve", disolve));
	AddItem(new TransitListItem("Flip", flip));
	AddItem(new TransitListItem("Gradient", gradient));
	AddItem(new TransitListItem("Swap", swap_transition));
	AddItem(new TransitListItem("Venetian Stripes", venetian_stripes));
	AddItem(new TransitListItem("Wipe", wipe));
	//fin initialisation.
	
	

}

void TransitListView::FixupScrollBars()
{
	BScrollBar *sb;
	BRect bounds;
	bounds = Bounds();
	float myPixelHeight = bounds.Height();
	float maxHeight = TRANSITIONBOX_HEIGHT - B_H_SCROLL_BAR_HEIGHT;
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

void TransitListView::FrameResized(float width, float height)
{
	FixupScrollBars();
}

TransitListItem::TransitListItem(char *transition, transition_type ftype) : BListItem()
{
	transition_name = (char *)malloc(strlen(transition) + 1);
	type = ftype;
	strcpy(transition_name, transition);
	preview = BTranslationUtils::GetBitmap(transition_name);
}

TransitListItem::~TransitListItem()
{
	delete transition_name;
	if (preview != NULL)
		delete preview;
}

void TransitListItem::DrawItem(BView *owner, BRect frame, bool complete = false)
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
	rgb_color	black_color = {0, 0, 0, 255};
	BRect	previewRect(frame.left + 2, frame.top +2, frame.left + 32, frame.bottom -3); //mettre en memoire
	owner->SetHighColor(owner->ViewColor());
	owner->SetHighColor(black_color); //on dessine en noir
	owner->StrokeRect(frame); //on dessine le rectangle autour de la preview
	owner->DrawBitmap(preview, previewRect);
	owner->MovePenTo(previewRect.right + 5, (frame.top + frame.bottom) / 2);
	owner->DrawString(transition_name);
}

void TransitListItem::Update(BView *owner, const BFont *font)
{
	BListItem::Update(owner, font);
	SetHeight(height);
}