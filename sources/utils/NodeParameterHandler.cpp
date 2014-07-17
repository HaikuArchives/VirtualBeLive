#include "NodeParameterHandler.h"
#include <MediaRoster.h>
#include <Messenger.h>


NodeParameterHandler::NodeParameterHandler(EventComposant *composant)
{
	status_t	err;
	node = NULL;
	test = NULL;
	fComposant = composant;
	fRoster = BMediaRoster::Roster();
	InstantiateNode();
	if (/*node*/test)
	{
		Run();
		messenger = new BMessenger(this);
		err = fRoster->StartWatching(*messenger, /*node*/test->Node(), B_MEDIA_NEW_PARAMETER_VALUE);
		err = fRoster->StartControlPanel(/*node*/test->Node());
	}
}

NodeParameterHandler::~NodeParameterHandler()
{
	fRoster->StopWatching(*messenger);
	delete messenger;
	fRoster->ReleaseNode(node->Node());
}

void NodeParameterHandler::MessageReceived(BMessage *message)
{
	switch (message->what)
	{
		case B_MEDIA_NEW_PARAMETER_VALUE:
			HandleMessage(message);
			break;
		default:
			BLooper::MessageReceived(message);
			break;
	}
}

void NodeParameterHandler::HandleMessage(BMessage *message)
{
	parameter_list_elem		*elem;
	int32					id;
	const void				*value;
	ssize_t					size;

	message->FindInt32("parameter", &id);
	message->FindData("value", B_RAW_TYPE, &value, &size);
	elem = new parameter_list_elem;
	elem->id = id;
	elem->value_size = size;
	elem->value = malloc(size);
	memcpy(elem->value, value, size);
	if (fComposant->event == filter)
		fComposant->u.filter.param_list->AddItem(elem);
	else
		fComposant->u.transition.param_list->AddItem(elem);
}

void NodeParameterHandler::InstantiateNode()
{
	if (fComposant->event == filter)
	{
		switch (fComposant->u.filter.type)
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
//				node = (BMediaNode*)(new EmbossFilter);
				test = new EmbossFilter;
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
	}else if (fComposant->event == transition)
	{
		switch (fComposant->u.transition.type)
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

void CreateParameterList(parameter_list **list)
{
	parameter_list *l = new parameter_list;
	
	
}








