#ifndef MENU_WIN_H
#define MENU_WIN_H

#include <Window.h>
#include <View.h>
#include <Menu.h>
#include <Bitmap.h>

class MenuWindow : public BWindow
{
public:
			MenuWindow();
			~MenuWindow();
private:
	void	InitMenu();
	BMenuBar	*menu;
	BMessage	*msg_begin;
};

class SplashScreen : public BWindow
{
public:
			SplashScreen();
			~SplashScreen();
	virtual void	WindowActivated(bool active);
private:
	BView	*splashView;
	BBitmap	*splashBitmap;
};

#endif