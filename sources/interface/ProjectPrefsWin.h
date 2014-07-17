#ifndef PROJECTPREFS_H
#define PROJECTPREFS_H

#include <Application.h>
#include <ListView.h>
#include <Rect.h>
#include <String.h>
#include <View.h>
#include <Window.h>
#include <TextControl.h>
#include <FilePanel.h>

class BMediaFile;
class BMenuField;
struct media_codec_info;
struct media_file_format;

struct Prefs
{
	entry_ref			saveFile;
	media_file_format	format;
	media_codec_info	video_codec;
	media_codec_info	audio_codec;
};

class ProjectPrefsWin : public BWindow
{
public:

				ProjectPrefsWin(BRect frame);
	virtual		~ProjectPrefsWin();

	void		BuildFormatMenu();
	void		BuildAudioVideoMenus();
	void		GetSelectedFormatInfo(media_file_format *format,
									 media_codec_info *audio,
									 media_codec_info *video);
	void		GetSelectedEntry(entry_ref *ref);
	
	
	void		SetEnabled(bool enabled, bool buttonEnabled);
	bool		IsEnabled();
	
protected:
	virtual void	DispatchMessage(BMessage *msg, BHandler *handler);
	virtual void	MessageReceived(BMessage *msg);
	virtual bool	QuitRequested();

private:
	media_format	fDummyFormat;
	BButton			*fConvertButton;
	BButton			*fChooseButton;
	BTextControl	*fileName;
	entry_ref		theFile;
	BEntry			entry;
	BMenuField		*fFormatMenu;
	BMenuField		*fVideoMenu;
	BMenuField		*fAudioMenu;
	BFilePanel		*fFilePanel;
	bool			fEnabled;
	bool			fConverting;
	bool			fCancelling;
};

#endif // MEDIACONVERTER_H