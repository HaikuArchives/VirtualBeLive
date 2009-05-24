#include "MenuWindow.h"
#include <TranslationUtils.h>
#include <Application.h>
#include <Screen.h>
#include <MenuBar.h>
#include <Menu.h>
#include <MenuItem.h>
#include "consts.h"

//#define DEBUG

MenuWindow::MenuWindow() 
	: BWindow(BRect(50.0, 50.0, 300.0, 60.0), 
			"VirtuaL BeLivE!", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	InitMenu();
	MoveTo(5, 25);
}

void MenuWindow::InitMenu()
{
	BMenu		*sub;
	BMenuItem	*item;
	
	BRect	menuRect = Bounds();
	menuRect.bottom = 10.0;
	menu = new BMenuBar(menuRect, "Main Menu");
	
	sub = new BMenu("File");
//	sub->AddItem(item = new BMenuItem("Open a project...", new BMessage(msg_OpenProject), 'O'));
//	item->SetTarget(be_app);
	sub->AddItem(item = new BMenuItem("Open a Project...", new BMessage(msg_OpenProject), 'O'));
	item->SetTarget(be_app);
	sub->AddItem(item = new BMenuItem("Open Recent Project...", new BMessage(msg_Open), 'R'));
	item->SetTarget(be_app);
	sub->AddItem(item = new BMenuItem("Save a Project...", new BMessage(msg_SaveProject), 'S'));
	item->SetTarget(be_app);
	sub->AddSeparatorItem();
	sub->AddItem(item = new BMenuItem("Import a Video...", new BMessage(msg_Import), 'I'));
	item->SetTarget(be_app); 
	sub->AddSeparatorItem();
	sub->AddItem(item = new BMenuItem("Project Settings", new BMessage(msg_Prefs), 'P'));
	item->SetTarget(be_app); 
	sub->AddSeparatorItem();
	sub->AddItem(item = new BMenuItem("Render", new BMessage(RENDER_MESSAGE), 'R', B_SHIFT_KEY));
	item->SetTarget(be_app);
	sub->AddSeparatorItem();
	sub->AddItem(item = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
	item->SetTarget(be_app);
	
	menu->AddItem(sub);
	
	sub = new BMenu("Edit");
	sub->AddItem(item = new BMenuItem("Copy", new BMessage(msg_Copy), 'C'));
	item->SetTarget(be_app);
	sub->AddItem(item = new BMenuItem("Cut", new BMessage(msg_Cut), 'X'));
	item->SetTarget(be_app);
	sub->AddItem(item = new BMenuItem("Paste", new BMessage(msg_Paste), 'V'));
	item->SetTarget(be_app);
	
	menu->AddItem(sub);
	
	sub = new BMenu("View / Hide");
	sub->AddItem(item = new BMenuItem("TimeLine BoX", new BMessage(msg_TimeBox), 'T'));
	item->SetTarget(be_app);
	sub->AddItem(item = new BMenuItem("Rush BoX", new BMessage(msg_RushBox), 'R'));
	item->SetTarget(be_app);
	sub->AddItem(item = new BMenuItem("Fx BoX", new BMessage(msg_FxBox), 'F'));
	item->SetTarget(be_app);
	sub->AddItem(item = new BMenuItem("TransitioN BoX", new BMessage(msg_TransitBox), 'A'));
	item->SetTarget(be_app);
	
	
	menu->AddItem(sub);	
	sub = new BMenu("Help");
	sub->AddItem(item = new BMenuItem("About...", new BMessage(msg_About)));
	item->SetTarget(be_app);
	menu->AddItem(sub);
#ifdef DEBUG
	sub = new BMenu("Debug");
	sub->AddItem(item = new BMenuItem("Start", new BMessage(START_MSG)));
	item->SetTarget(be_app);
	sub->AddItem(item = new BMenuItem("Stop", new BMessage(STOP_MSG)));
	item->SetTarget(be_app);
	sub->AddItem(item = new BMenuItem("SetRef", new BMessage(SET_REF)));
	item->SetTarget(be_app);

	menu->AddItem(sub);
#endif
	AddChild(menu);
}

MenuWindow::~MenuWindow()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
}

SplashScreen::SplashScreen()
	: BWindow(BRect(0, 0, kVBLlogoWidth - 1, kVBLlogoHeight + 1), "SplashScreen", B_BORDERED_WINDOW, 
							B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_NOT_MOVABLE | B_NOT_ZOOMABLE)
{
	BScreen		*theScreen = new BScreen(this);
	BRect		screenRect = theScreen->Frame();
	
	MoveTo(screenRect.Width() / 2 - Frame().Width() / 2, screenRect.Height() / 2 - Frame().Height() / 2);
	delete theScreen;
}

void SplashScreen::WindowActivated(bool active)
{
	if (active)
	{
		BRect	rect = Bounds();
		splashView = new BView(rect, "splashview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
		AddChild(splashView);
		splashBitmap = BTranslationUtils::GetBitmap("VBLogo");
		splashView->SetViewBitmap(splashBitmap);
	}
}

SplashScreen::~SplashScreen()
{

}