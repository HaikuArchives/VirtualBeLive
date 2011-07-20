#include "draw.h"
#include "draw_window.h"
#include "MenuWindow.h"
#include <FilePanel.h>
#include <Path.h>
#include <stdio.h>
#include <Screen.h>
#include <Bitmap.h>
#include <Alert.h>
#include <MediaKit.h>
#include "VirtualRenderer.h"

int32 DrawApp::sNumWindows = 0;

//#define TEST_TRANSITION


DrawApp::DrawApp()
	: BApplication("application/x-vnd.Be.MediaPlayer")
{
	fOpenPanel = NULL;
	fSelectPanel = NULL;
	
	renderer = NULL;
}


DrawApp::~DrawApp()
{
	delete (fOpenPanel);
	delete (fSelectPanel);
	delete (fSavePanel);
	delete (fOpenProjectPanel);
//	roster->StartNode(filter->Node(), 0);

/*	delete filter2;
	delete filter3;
	delete filter4;
	delete filter5;
	delete filter6;
	delete filter7;
	delete filter8;
*/	
	
}

void
DrawApp::OpenSelectPanel() //choix de l'import ds la rush box
{
	if (fSelectPanel == NULL)
	{
		fSelectPanel = new BFilePanel();
		fSelectPanel->Window()->SetTitle("Select a file to import:");
		fSelectPanel->SetButtonLabel(B_DEFAULT_BUTTON, "Import");
		fSelectPanel->SetMessage(new BMessage(msg_ImportRush)); //changement du message envoyé
		fSelectPanel->SetTarget(Rush);
	}
	fSelectPanel->Show();
}

void
DrawApp::OpenOpenPanel()
{
	if (fOpenPanel != NULL)
		return;

	fOpenPanel = new BFilePanel();
	fOpenPanel->Window()->SetTitle("Select a project to open:");
	fOpenPanel->Show();
}

void DrawApp::OpenOpenProjectPanel()
{
	if (fOpenProjectPanel == NULL)
	{
		fOpenProjectPanel = new BFilePanel();
		fOpenProjectPanel->Window()->SetTitle("Select a project to open:");
		fOpenProjectPanel->Show();
	}
}

void DrawApp::OpenSavePanel()
{
	if (fSavePanel == NULL)
	{
		fSavePanel = new BFilePanel(B_SAVE_PANEL);
		fSavePanel->Window()->SetTitle("Save the project to:");
		fSavePanel->SetMessage(new BMessage(msg_Save));
		fSavePanel->SetButtonLabel(B_DEFAULT_BUTTON, "Save");
		fSavePanel->SetTarget(be_app);
	}
	fSavePanel->Show();
}

status_t DrawApp::Save(BMessage *message)
{
	entry_ref	ref;
	const char	*name;
	status_t	err = B_OK;
	
	if ((err = message->FindRef("directory", &ref)) != B_OK)
		return err;
	if ((err = message->FindString("name", &name)) != B_OK)
	{
		return err;
		BDirectory dir(&ref);
		if ((err = dir.InitCheck()) != B_OK)
			return err;
		BFile file(&dir, name, B_READ_WRITE | B_CREATE_FILE);
		if ((err = file.InitCheck()) != B_OK)
			return err;
	}
	return err;
}

void
DrawApp::ReadyToRun()
{
	fMenuWin = new MenuWindow();
	fMenuWin->Show();
	sNumWindows++;
//	OpenOpenPanel();
	(TimeBox = new TimeWin)->Show();
	(Rush = new rushwin)->Show(); //on affiche la rush
	(FxBox = new Fxwin)->Show();	//on affiche la Fx BoX
	(TransitBox = new Transitwin)->Show();	//on affiche la TransitBox
	PopUp = new PopUpWin;
	prefsWin = new ProjectPrefsWin(BRect(200, 250, 500, 410));

	roster = BMediaRoster::Roster();
//	media_node *timesourceNode = new media_node;
//	roster->GetTimeSource(timesourceNode);
//	timeSource = roster->MakeTimeSourceFor(*timesourceNode);
//
//	filter = new TestFilter(NULL);
//	transition = new TestTransition(NULL);
//	roster->SetTimeSourceFor(filter->Node().node, timeSource->Node().node);
//	roster->SetTimeSourceFor(transition->Node().node, timeSource->Node().node);
//	
//	reader = new FileReader("Memorial Lower", VIDEO_PATH, 0);
//	roster->SetTimeSourceFor(reader->Node().node, timeSource->Node().node);
//	reader2 = new FileReader("Memorial Upper", "/boot/optional/movies/Memorial Day (Upper)", 0);
//	roster->SetTimeSourceFor(reader2->Node().node, timeSource->Node().node);
//
//
//	entry_ref	fileref;
//	BEntry		entry("/boot/home/test.avi");
//	entry.GetRef(&fileref);
//
////	writer = new VideoTest(fileref, 0);
//	
//	media_input	inputs[2];
//	media_output	outputs;
//	media_format	format;
//	int32	nb = 1;
//#ifdef TEST_TRANSITION	
//	roster->GetFreeOutputsFor(reader->Node(), &outputs, 1, &nb, B_MEDIA_RAW_VIDEO);
//	roster->GetFormatFor(outputs, &format);
//	roster->GetFreeInputsFor(transition->Node(), inputs, 2, &nb, B_MEDIA_RAW_VIDEO);
//	roster->Connect(outputs.source, inputs[0].destination, &format, &outputs, inputs);
//	roster->GetFreeOutputsFor(reader2->Node(), &outputs, 1, &nb, B_MEDIA_RAW_VIDEO);
//	roster->GetFormatFor(outputs, &format);
//	roster->Connect(outputs.source, inputs[1].destination, &format, &outputs, &inputs[1]);
//	roster->GetVideoOutput(&dispNode);
//	roster->SetTimeSourceFor(dispNode.node, timeSource->Node().node);
//	roster->GetFreeInputsFor(dispNode, inputs, 1, &nb, B_MEDIA_RAW_VIDEO);
//	roster->GetFreeOutputsFor(transition->Node(), &outputs, 1, &nb, B_MEDIA_RAW_VIDEO);
//	roster->Connect(outputs.source, inputs[0].destination, &format, &outputs, inputs);
//	
//#endif

/*	filter2 = new SlowFilter(NULL);
	filter3 = new InvertFilter(NULL);
	filter4 = new GrayFilter(NULL);
	filter5 = new SolarFilter(NULL);
	filter6 = new ContrastFilter(NULL);
	filter7 = new MixFilter(NULL);
	filter8 = new ColorFilter2(NULL);
*/

}


void
DrawApp::ArgvReceived(
	int32	argc,
	char	**argv)
{
//	for (int32 i = 1; i < argc; i++) {
//		sNumWindows++;
//		(new DrawWindow(BRect(200.0, 200.0, 500.0, 500.0), argv[i]))->Show();
//	}
}


void
DrawApp::MessageReceived(BMessage	*message)
{
	BEntry		entry("/boot/optional/movies/Memorial Day (Lower)");
	BPath		path;
	entry_ref	fileref;
	media_input	inputs[2];
	media_output	outputs;
	media_format	format;
	int32	nb = 1;
	void	*pointer;

	bigtime_t now;
	switch (message->what) {
		case B_SILENT_RELAUNCH:
			OpenOpenPanel();
			break;

		case B_CANCEL:
			if (fOpenPanel != NULL) {
				delete (fOpenPanel);
				fOpenPanel = NULL;
				
				if (sNumWindows < 1)
					PostMessage(B_QUIT_REQUESTED);
			}
			break;
		case msg_Save:
			Save(message);
			break;
		case msg_Open:
			OpenOpenPanel();
			break;
		case msg_About:
			(new BAlert("About", "Virtual BeLive!\n\n Open Source Video Editing Software by Bill Davenport \n Origional Creators: EwnS!, STeF and Interseb\n BeLive! 1999/2004", "OK!"))->Go();
			break;
		case msg_WindowClosed:
			if (--sNumWindows < 1) {
				if (fOpenPanel == NULL)
					PostMessage(B_QUIT_REQUESTED);
				else
					sNumWindows = 0;
			}
			break;
		case msg_TimeBox:
			if (TimeBox->IsHidden() == false)
				TimeBox->Hide();
			else
				TimeBox->Show();
			break;
			
		case msg_RushBox:
			if (Rush->IsHidden() == false)
				Rush->Hide();
			else
				Rush->Show();
			break;
			
		case msg_FxBox:
			if (FxBox->IsHidden() == false)
				FxBox->Hide();
			else
				FxBox->Show();
			break;
			
		case msg_TransitBox:
			if (TransitBox->IsHidden() == false)
				TransitBox->Hide();
			else
				TransitBox->Show();
			break;
		case msg_Import:
			OpenSelectPanel();
			break;
		case msg_SaveProject:
			OpenSavePanel();
			break;
			
		case msg_OpenProject:
			OpenOpenProjectPanel();
			break;
			
		case msg_PopUp:
			message->FindPointer("Composant", &pointer);
			PopUp->Adjust((EventComposant*)pointer);
			PopUp->Show();

			break;
			
		case msg_PopUpDraw:
			if (TimeBox->Lock())
			{
				TimeBox->tView->Invalidate();
				TimeBox->Unlock();
			}	
			break;
		case START_MSG:
			now = timeSource->PerformanceTimeFor(BTimeSource::RealTime() + 2000000 / 50);
//			roster->StopNode(reader2->Node(), now, true);
//			roster->StopNode(reader->Node(), now, true);
			roster->StartNode(reader2->Node(), now);
			roster->StartNode(reader->Node(), now);
			roster->StartNode(transition->Node(), now);
			roster->StartNode(dispNode, now);
			break;
		case STOP_MSG:
			now = timeSource->PerformanceTimeFor(BTimeSource::RealTime() + 1000000 / 50);
			roster->StopNode(reader2->Node(), now, true);
			roster->StopNode(reader->Node(), now, true);
			roster->StopNode(transition->Node(), now, true);
			roster->StopNode(dispNode, now, true);		
			break;
		case MEDIA_FORMATS:	
//			entry.GetRef(&fileref);
//			entry.GetPath(&path);
			prefsWin->GetSelectedFormatInfo(&prefs.format, &prefs.audio_codec, &prefs.video_codec);
			prefsWin->GetSelectedEntry(&prefs.saveFile);
//			reader = new FileReader("Reader", path.Path(), 0);
//			writer = new DiskWriter(prefs.saveFile, prefs.format, prefs.video_codec, prefs.audio_codec, 0);
//			roster->SetRefFor(writer->Node(), prefs.saveFile, true, &now);
			break;
		case SET_REF:
			//now = timeSource->PerformanceTimeFor(BTimeSource::RealTime() + 2000000 / 50);
			roster->GetFreeOutputsFor(reader->Node(), &outputs, 1, &nb, B_MEDIA_RAW_VIDEO);
			roster->GetFormatFor(outputs, &format);			
			roster->GetFreeInputsFor(writer->Node(), inputs, 1, &nb, B_MEDIA_RAW_VIDEO);
			roster->Connect(outputs.source, inputs[0].destination, &format, &outputs, inputs);
//			roster->GetFreeOutputsFor(reader->Node(), &outputs, 1, &nb, B_MEDIA_RAW_AUDIO);
//			roster->GetFormatFor(outputs, &format);			
//			roster->GetFreeInputsFor(writer->Node(), inputs, 1, &nb, B_MEDIA_RAW_AUDIO);
//			roster->Connect(outputs.source, inputs[0].destination, &format, &outputs, inputs);		
//			roster->SetRefFor(writer->Node(), fileref, true, &now);
			roster->SetRunModeNode(writer->Node(), BMediaNode::B_OFFLINE);
			roster->SetRunModeNode(reader->Node(), BMediaNode::B_OFFLINE);
			roster->RollNode(reader->Node(), 0, 10000000);
			roster->RollNode(writer->Node(), 0, 10000000);
			roster->StartWatching(be_app, writer->Node(), B_MEDIA_NODE_STOPPED);
			break;
		case B_MEDIA_NODE_STOPPED:
//			delete writer;
//			delete reader;
			break;
		case RENDER_MESSAGE:
			renderer = new VirtualRenderer(TimeBox->tView->List);
			renderer->Start();
			break;
		case msg_Prefs:
			prefsWin->Show();
			break;
		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void DrawApp::RefsReceived(BMessage	*message)
{
	int32	count = 0;
	uint32	type = 0;
	message->GetInfo("refs", &type, &count);

	for (int32 i = 0; i < count; i++) {
		entry_ref	ref;

		if (message->FindRef("refs", i, &ref) == B_NO_ERROR) {
			BEntry entry(&ref);

			if (entry.InitCheck() == B_NO_ERROR) {
				BPath path;
				entry.GetPath(&path);

				sNumWindows++;
//				(new DrawWindow(BRect(300.0, 200.0, 600.0, 500.0), path.Path()))->Show();
				PlayFile(path.Path());
			}
		}
	}
}


void DrawApp::PlayFile(const char *path)
{
	status_t	err;
	media_node	*outputNode;
	outputNode = new media_node;
	FileReader *prod = new FileReader(path, path, 0);
	roster->RegisterNode(prod);
	if ((err = roster->GetVideoOutput(outputNode)))
		return;
	media_output	*output = new media_output;
	media_input		*input = new media_input;
	int32			nb = 0;
	roster->GetFreeInputsFor(*outputNode, input, 1, &nb, B_MEDIA_RAW_VIDEO);
	if (!nb) // Plus de ports libres...
		return;
	nb = 0;
	roster->GetFreeOutputsFor(prod->Node(), output, 1, &nb, B_MEDIA_RAW_VIDEO);
	if (!nb) // Plus de ports libres...
		return;
		
	media_format	try_format;
	roster->GetFormatFor(*output, &try_format);
	
	if ((err = roster->Connect(output->source, input->destination, &try_format, output, input)))
	{
		(new BAlert("Attention", strerror(err), "OK"))->Show();
		return;
	}
	bigtime_t	start_time = 0;
	if ((err = roster->SetTimeSourceFor(prod->Node().node, timeSource->Node().node)))
		return;
	if ((err = roster->SetTimeSourceFor(outputNode->node, timeSource->Node().node)))
		return;
//	err = roster->GetStartLatencyFor(prod->Node(), &start_time);
	start_time += timeSource->PerformanceTimeFor(BTimeSource::RealTime() + 2000000 / 50);
	err = roster->StartNode(prod->Node(), start_time);
	err = roster->StartNode(*outputNode, start_time);
	
	
}

bool DrawApp::QuitRequested()
{
	/*delete filter;
	delete writer;
	delete reader;
	delete reader2;
	delete transition;*/
//	roster->ReleaseNode(filter->Node());
//	roster->ReleaseNode(writer->Node());
//	roster->ReleaseNode(reader->Node());
//	roster->ReleaseNode(reader2->Node());
//	roster->ReleaseNode(transition->Node());
	return BApplication::QuitRequested();
}

int main(int argc, char **argv)
{
	DrawApp *app = new DrawApp();
	app->Run();
	
	delete (app);
			
	return (B_NO_ERROR);
}
