#include "FxBox.h"
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


Fxwin::Fxwin() : BWindow(BRect(0.0, 0.0, 0.0, 0.0), "Fx BoX", B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_CLOSABLE | B_NOT_H_RESIZABLE | B_ASYNCHRONOUS_CONTROLS)
{
	//on prend la resolution de l'ecran afin de bien placer les fenetres.
	BScreen		*theScreen = new BScreen(this);
	BRect		r = theScreen->Frame();
	
	//fenetre...
	SetZoomLimits(FXBOX_WIDTH, FXBOX_HEIGHT);
	r.InsetBy(5,5);
	ResizeTo(FXBOX_WIDTH, FXBOX_HEIGHT);
	MoveTo(r.right - (FXBOX_WIDTH + 5), r.top + 15);

	//vues...
	r= Bounds();
	r.right -= B_V_SCROLL_BAR_WIDTH;

	BView *owner;
	FxView = new FxListView(r,"List");
	AddChild(new BScrollView("Main View", FxView, B_FOLLOW_ALL_SIDES, B_WILL_DRAW, false, true));
	
	}

Fxwin::~Fxwin()
{

}

FxListView::FxListView(BRect frame, char *name) : BListView(frame, name, B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS)
{
	
}

FxListView::~FxListView()
{

}


bool FxListView::InitiateDrag(BPoint point, int32 index, bool wasSelected)
{
	bool	result = wasSelected;
	if ( result )
	{
		BMessage message(msg_FxDropped);
		BRect	selectRect = Bounds();
		float	itemSize = ItemAt(index)->Height();
		selectRect.top = index * itemSize;
		selectRect.bottom = selectRect.top + itemSize;
		char *ref = ((FxListItem*)ItemAt(index))->filter_name;
		message.AddInt32("filter_width", 10000000);
		message.AddInt32("filter_type", ((FxListItem*)ItemAt(index))->type);
		message.AddString("ref", (char *)ref);
		DragMessage(&message, selectRect);		
	}
	return result;

}

void FxListView::AttachedToWindow(void)
{
	FixupScrollBars();
	parent = (Fxwin*) Window();
	// initialisation des filtres primaires.
	AddItem(new FxListItem("Black and White", black_white));
	AddItem(new FxListItem("Blur", blur));
	AddItem(new FxListItem("Brightness / Contrast",contrast_brightness));
	AddItem(new FxListItem("Color", rgb_channel));
	AddItem(new FxListItem("Emboss", emboss));
	AddItem(new FxListItem("Gray", gray));
	AddItem(new FxListItem("Level", levels));
	AddItem(new FxListItem("Mix", mix));
	AddItem(new FxListItem("Negatif", invert));
	AddItem(new FxListItem("Offset", offset));
	AddItem(new FxListItem("Pixelisation", mozaic));
	AddItem(new FxListItem("Solar", solarize));
	AddItem(new FxListItem("Trame", trame));
	AddItem(new FxListItem("Binary Operator", frame_bin_op));
	AddItem(new FxListItem("Mirror", hv_mirror));
	AddItem(new FxListItem("RGB Intensity", rgb_intensity));
	AddItem(new FxListItem("Motion Blur", motion_blur));
	AddItem(new FxListItem("Motion BW Threshold", motion_bw_threshold));
	AddItem(new FxListItem("Motion RGB Threshold", motion_rgb_threshold));
	AddItem(new FxListItem("Motion Mask Threshold", motion_mask));
	AddItem(new FxListItem("Step Motion", step_motion));
	AddItem(new FxListItem("Diff Detection", diff_detection));
	AddItem(new FxListItem("RGB Threshold", rgb_threshold));
	//fin initialisation.
	
	

}

void FxListView::FixupScrollBars()
{
	BScrollBar *sb;
	BRect bounds;
	bounds = Bounds();
	float myPixelHeight = bounds.Height();
	float maxHeight = FXBOX_HEIGHT - B_H_SCROLL_BAR_HEIGHT;
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

void FxListView::FrameResized(float width, float height)
{
	FixupScrollBars();
}

FxListItem::FxListItem(char *filter, filter_type ftype) : BListItem()
{
	filter_name = (char *)malloc(strlen(filter) + 1);
	type = ftype;
	strcpy(filter_name, filter);
	preview = BTranslationUtils::GetBitmap(filter_name);
}

FxListItem::~FxListItem()
{
	delete filter_name;
	if (preview != NULL)
		delete preview;
}

void FxListItem::DrawItem(BView *owner, BRect frame, bool complete = false)
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
	BRect	previewRect(frame.left + 2, frame.top +2, frame.left + 32, frame.bottom -3); //mettre en memoire
	owner->SetHighColor(owner->ViewColor());
	//owner->FillRect(frame);
	owner->SetHighColor(black); //on dessine en noir
	owner->StrokeRect(frame); //on dessine le rectangle autour de la preview
	owner->DrawBitmap(preview, previewRect);
	owner->MovePenTo(previewRect.right + 5, (frame.top + frame.bottom) / 2);
	owner->DrawString(filter_name);
}

void FxListItem::Update(BView *owner, const BFont *font)
{
	BListItem::Update(owner, font);
	SetHeight(height);
}