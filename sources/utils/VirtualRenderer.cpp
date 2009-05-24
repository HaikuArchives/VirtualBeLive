#include "VirtualRenderer.h"
#include "ProjectPrefsWin.h"
#include "draw.h"
#include "AllNodes.h"

#include <MediaKit.h>

VirtualRenderer::VirtualRenderer(EventList *list) : BLooper()
{
	eventList = list;
	prefs = &((DrawApp*)be_app)->prefs;

	writer = NULL;
	reader1 = NULL;
	reader2 = NULL;
//	nullgen = NULL;

	Run();
}

VirtualRenderer::~VirtualRenderer()
{

}

status_t VirtualRenderer::Start()
{
	if ((lock_sem = create_sem(1, "Lock Sem")) < B_OK)
		return B_ERROR;
//	if (acquire_sem(lock_sem) != B_OK)
//		return B_ERROR;
	renderThread = spawn_thread(renderstart, "Renderer Thread", B_NORMAL_PRIORITY, this);
	if (renderThread < B_OK)
		return B_ERROR;
	return resume_thread(renderThread);
}

int compare(const void *first, const void *second)
{
	return ((**(bigtime_t**)first) - (**(bigtime_t**)second));
}

int32 VirtualRenderer::RenderLoop()
{
	BList			timeList(1);
	EventComposant 	*composant;
	bigtime_t		*time, *time2, duration;
	
	bool		firstTrackActive = true;
	int			i, j, timeEvent;
	roster = BMediaRoster::Roster();
	
	/* Retrieve all the times where an event occur (starts and stops) in the list */
	for (i = 0; i < eventList->CountItems(); i++)
	{
		composant = eventList->ItemAt(i);
		composant->startRendered = false;
		composant->endRendered = false;
		time = new bigtime_t;
		*time = composant->time;
		timeList.AddItem(time);
		time = new bigtime_t;
		*time = composant->time + composant->end;
		timeList.AddItem(time);
	}
	timeList.SortItems(compare);
	/*  Instantiate a writer node */
	writer = new DiskWriter(prefs->saveFile, prefs->format, prefs->video_codec, prefs->audio_codec, 0);
	roster->SetRefFor(writer->Node(), prefs->saveFile, true, &duration);
	BMessenger	*messenger = new BMessenger(this);;
	roster->SetRunModeNode(writer->Node(), BMediaNode::B_OFFLINE);
	roster->StartWatching(*messenger, writer->Node(), B_MEDIA_NODE_STOPPED);
	/* 	if there is not video available at the beginning, we need a null generator
		to fill in the beginning of the file, else we enter the loop	*/
	//	if (eventList->FirstItem()->time != 0)
//	{
//		nullgen = new NullGen;
//		
//	}
	timeEvent = 0;
	media_output		outputs[3];
	int32				numOutputs;
	media_input			inputs[4];
	int32				numInputs;
	media_source		sources[2];
	media_destination	destinations[2];
	media_format		format;
	media_node			filterNode, transitionNode;
	bool				doConnect = false;
	bool				writerConnected = false;
	bool				reader1Connected = false;
	bool				reader2Connected = false;
	bool				filterConnected = false;
	bool				transitionConnected = false;
	
	for (timeEvent = 0; timeEvent < timeList.CountItems(); timeEvent++)
	{
		if (acquire_sem(lock_sem) == B_OK)
		{
			i = 0;
//			while (*((bigtime_t*)timeList.ItemAt(timeEvent)) != eventList->ItemAt(i)->time)
//				i++;
			duration = *((bigtime_t*)timeList.ItemAt(timeEvent));
			while (i < eventList->CountItems())
			{
				composant = eventList->ItemAt(i);
				if ((composant->time == duration) && !composant->startRendered)
				{
					composant->startRendered = true;
					break;
				}
				if ((composant->time + composant->end == duration) && !composant->endRendered)
				{
					composant->endRendered = true;
					break;
				}
				i++;
			}
			/*  We must find the events that just terminated */
			for (j = i; j < eventList->CountItems(); j++)
			{
				composant = (EventComposant*)eventList->ItemAt(j);
				if (composant->time + composant->end == *(bigtime_t*)timeList.ItemAt(timeEvent))
				switch (composant->event)
				{
					case video1:
						roster->ReleaseNode(reader1->Node());
						reader1 = NULL;
						reader1Connected = false;
						break;
					case video2:
						roster->ReleaseNode(reader2->Node());
						reader2 = NULL;
						reader2Connected = false;
						break;
					case transition:
						firstTrackActive = !firstTrackActive;
						roster->GetConnectedInputsFor(filterNode, &inputs[0], 2, &numInputs);
						roster->Disconnect(reader1->Node().node, inputs[0].source, transitionNode.node, inputs[0].destination);
						roster->Disconnect(reader2->Node().node, inputs[1].source, transitionNode.node, inputs[1].destination);
						
						roster->GetConnectedOutputsFor(transitionNode, &outputs[0], 1, &numOutputs);
						if (filterConnected)
						{
							roster->Disconnect(transitionNode.node, outputs[0].source, filterNode.node, outputs[0].destination);
							if (firstTrackActive)
								roster->Connect(inputs[0].source, outputs[0].destination, &inputs[0].format, &outputs[0], &inputs[0]);
							else
								roster->Connect(inputs[1].source, outputs[0].destination, &inputs[1].format,  &outputs[0], &inputs[1]);
						}
						else
						{
							roster->Disconnect(transitionNode.node, outputs[0].source, writer->Node().node, outputs[0].destination);
							if (firstTrackActive)
								roster->Connect(inputs[0].source, outputs[0].destination, &inputs[0].format, &outputs[0], &inputs[0]);
							else
								roster->Connect(inputs[1].source, outputs[0].destination, &inputs[1].format, &outputs[0], &inputs[1]);						
						}
						transitionConnected = false;
						break;
					case filter:
						roster->GetConnectedInputsFor(filterNode, &inputs[0], 1, &numInputs);
						if (transitionConnected)
						{
							roster->Disconnect(transitionNode.node, inputs[0].source, filterNode.node, inputs[0].destination);
							roster->GetConnectedOutputsFor(filterNode, &outputs[0], 1, &numOutputs);
							roster->Disconnect(filterNode.node, outputs[0].source, writer->Node().node, outputs[0].destination);
							roster->Connect(inputs[0].source, outputs[0].destination, &inputs[0].format, &outputs[0], &inputs[0]);
							filterConnected = false;
						}
						else if (firstTrackActive)
						{
							roster->Disconnect(reader1->Node().node, inputs[0].source, filterNode.node, inputs[0].destination);
							roster->GetConnectedOutputsFor(filterNode, &outputs[0], 1, &numOutputs);
							roster->Disconnect(filterNode.node, outputs[0].source, writer->Node().node, outputs[0].destination);
							roster->Connect(inputs[0].source, outputs[0].destination, &inputs[0].format, &outputs[0], &inputs[0]);
							filterConnected = false;							
						}
						else if (!firstTrackActive)
						{
							roster->Disconnect(reader2->Node().node, inputs[0].source, filterNode.node, inputs[0].destination);
							roster->GetConnectedOutputsFor(filterNode, &outputs[0], 1, &numOutputs);
							roster->Disconnect(filterNode.node, outputs[0].source, writer->Node().node, outputs[0].destination);
							roster->Connect(inputs[0].source, outputs[0].destination, &inputs[0].format, &outputs[0], &inputs[0]);
							filterConnected = false;							
						}
						
						
						break;
					default:
						break;
				}
			}
			if (timeEvent == timeList.CountItems() - 1)
				break;
		
			doConnect = false;
			composant = eventList->ItemAt(i);
			switch (composant->event)
			{
				case video1:
					if (reader1 != NULL)
					{
						roster->GetConnectedOutputsFor(reader1->Node(), outputs, 1, &numOutputs);
						if (numOutputs == 1)
						{
							destinations[0] = outputs[0].destination;
							doConnect = true;
						}
						//roster->ReleaseNode(reader1->Node());
						delete reader1;
						reader1Connected = false;
						reader1 = NULL;
					}
					reader1 = new FileReader(composant->u.video.filepath, composant->u.video.filepath, 0);
					roster->SetRunModeNode(reader1->Node(), BMediaNode::B_OFFLINE);
					if (doConnect)
					{
						roster->GetFreeOutputsFor(reader1->Node(), outputs, 1, &numOutputs);
						roster->Connect(outputs[0].source, destinations[0], &outputs[0].format, &outputs[0], &inputs[0]);
						reader1Connected = true;
					}
					else if (transitionConnected)
					{
						roster->GetFreeInputsFor(transitionNode, inputs, 1, &numInputs);
						roster->GetFreeOutputsFor(reader1->Node(), outputs, 1, &numOutputs);
						roster->GetFormatFor(outputs[0], &format);
						roster->Connect(outputs[0].source, inputs[0].destination, &format, &outputs[0], &inputs[0]);
						reader1Connected = true;
					}
					else if (filterConnected)
					{
						roster->GetFreeInputsFor(filterNode, inputs, 1, &numInputs);
						roster->GetFreeOutputsFor(reader1->Node(), outputs, 1, &numOutputs);
						roster->GetFormatFor(outputs[0], &format);
						roster->Connect(outputs[0].source, inputs[0].destination, &format, &outputs[0], &inputs[0]);
						reader1Connected = true;					
					}
					else if (!writerConnected && firstTrackActive)
					{
						roster->GetFreeInputsFor(writer->Node(), inputs, 1, &numInputs);
						roster->GetFreeOutputsFor(reader1->Node(), outputs, 1, &numOutputs);
						roster->GetFormatFor(outputs[0], &format);
						roster->Connect(outputs[0].source, inputs[0].destination, &format, &outputs[0], &inputs[0]);
						writerConnected = true;
						reader1Connected = true;
					}
					time = (bigtime_t*)timeList.ItemAt(timeEvent);
					time2 = (bigtime_t*)timeList.ItemAt(timeEvent + 1);
					
					/* Tell the nodes to start until next time event */
					roster->RollNode(writer->Node(), *time, *time2);
					if (filterConnected)
						roster->RollNode(filterNode, *time, *time2);
					if (transitionConnected)
						roster->RollNode(transitionNode, *time, *time2);
					roster->RollNode(reader1->Node(), *time, *time2, composant->u.video.begin);
					break;
				case video2:
					if (reader2 != NULL)
					{
						roster->GetConnectedOutputsFor(reader2->Node(), outputs, 1, &numOutputs);
						if (numOutputs == 1)
						{
							destinations[0] = outputs[0].destination;
							doConnect = true;
						}
						//roster->ReleaseNode(reader2->Node());
						delete reader2;
						reader2Connected = false;
						reader2 = NULL;
					}
					reader2 = new FileReader(composant->u.video.filepath, composant->u.video.filepath, 0);
					roster->SetRunModeNode(reader2->Node(), BMediaNode::B_OFFLINE);
					if (doConnect)
					{
						roster->GetFreeOutputsFor(reader2->Node(), outputs, 1, &numOutputs);
						roster->Connect(outputs[0].source, destinations[0], &outputs[0].format, &outputs[0], &inputs[0]);
						reader2Connected = true;
					}
//					else if (transitionConnected)
//					{
//						roster->GetFreeInputsFor(transitionNode, inputs, 1, &numInputs);
//						roster->GetFreeOutputsFor(reader2->Node(), outputs, 1, &numOutputs);
//						roster->GetFormatFor(outputs[0], &format);
//						roster->Connect(outputs[0].source, inputs[0].destination, &format, &outputs[0], &inputs[0]);
//						reader2Connected = true;
//					}
//					else if (filterConnected)
//					{
//						roster->GetFreeInputsFor(filterNode, inputs, 1, &numInputs);
//						roster->GetFreeOutputsFor(reader2->Node(), outputs, 1, &numOutputs);
//						roster->GetFormatFor(outputs[0], &format);
//						roster->Connect(outputs[0].source, inputs[0].destination, &format, &outputs[0], &inputs[0]);
//						reader2Connected = true;					
//					}
//					else if (!writerConnected && !firstTrackActive)
//					{
//						roster->GetFreeInputsFor(writer->Node(), inputs, 1, &numInputs);
//						roster->GetFreeOutputsFor(reader2->Node(), outputs, 1, &numOutputs);
//						roster->GetFormatFor(outputs[0], &format);
//						roster->Connect(outputs[0].source, inputs[0].destination, &format, &outputs[0], &inputs[0]);
//						writerConnected = true;
//						reader2Connected = true;
//					}
					time = (bigtime_t*)timeList.ItemAt(timeEvent);
					time2 = (bigtime_t*)timeList.ItemAt(timeEvent + 1);
					
					roster->RollNode(writer->Node(), *time, *time2);
					if (filterConnected)
						roster->RollNode(filterNode, *time, *time2);
					if (transitionConnected)
						roster->RollNode(transitionNode, *time, *time2);
					if (reader1Connected && firstTrackActive)
						roster->RollNode(transitionNode, *time, *time2);
					if (reader2Connected && !firstTrackActive)
						roster->RollNode(reader2->Node(), *time, *time2, composant->u.video.begin);
					break;
				case filter:
				/* Handle filter events */
				if (filterConnected)
					{
						roster->GetConnectedOutputsFor(filterNode, outputs, 1, &numOutputs);
						if (numOutputs == 1)
						{
							destinations[0] = outputs[0].destination;
							doConnect = true;
						}
						roster->GetConnectedInputsFor(filterNode, inputs, 1, &numInputs);
						if (numInputs == 1)
						{
							sources[0] = inputs[0].source;
						}
						roster->ReleaseNode(filterNode);
						filterConnected = false;
					}
					FindFilter(composant->u.filter.type, &filterNode);
					SetFilterParameters(filterNode, composant->u.filter.param_list);
					roster->SetRunModeNode(filterNode, BMediaNode::B_OFFLINE);
					if (doConnect)
					{
						roster->GetFreeOutputsFor(filterNode, &outputs[1], 1, &numOutputs);
						roster->Connect(outputs[1].source, destinations[0], &outputs[0].format, &outputs[1], &inputs[1]);
						filterConnected = true;
					}
					else if (transitionConnected)
					{
						roster->GetConnectedOutputsFor(transitionNode, &outputs[0], 1, &numOutputs);
						roster->Disconnect(transitionNode.node, outputs[0].source, writer->Node().node, outputs[0].destination);
						roster->GetFreeInputsFor(filterNode, &inputs[0], 1, &numInputs);
						roster->Connect(outputs[0].source, inputs[0].destination, &outputs[0].format, &outputs[0], &inputs[0]);
						roster->GetFreeOutputsFor(filterNode, &outputs[1], 1, &numOutputs);
						roster->GetFreeInputsFor(writer->Node(), &inputs[1], 1, &numInputs);
						roster->Connect(outputs[1].source, inputs[1].destination, &inputs[1].format, &outputs[1], &inputs[1]);
						filterConnected = true;
					}
					else if (reader1Connected && firstTrackActive)
					{
						roster->GetConnectedOutputsFor(reader1->Node(), &outputs[0], 1, &numOutputs);
						roster->Disconnect(reader1->Node().node, outputs[0].source, writer->Node().node, outputs[0].destination);
						roster->GetFreeInputsFor(filterNode, &inputs[0], 1, &numInputs);
						roster->Connect(outputs[0].source, inputs[0].destination, &outputs[0].format, &outputs[0], &inputs[0]);
						roster->GetFreeOutputsFor(filterNode, &outputs[1], 1, &numOutputs);
						roster->GetFreeInputsFor(writer->Node(), &inputs[1], 1, &numInputs);
						roster->Connect(outputs[1].source, inputs[1].destination, &inputs[1].format, &outputs[1], &inputs[1]);
						filterConnected = true;
					}
					else if (reader2Connected && !firstTrackActive)
					{
						roster->GetConnectedOutputsFor(reader2->Node(), &outputs[0], 1, &numOutputs);
						roster->Disconnect(reader2->Node().node, outputs[0].source, writer->Node().node, outputs[0].destination);
						roster->GetFreeInputsFor(filterNode, &inputs[0], 1, &numInputs);
						roster->Connect(outputs[0].source, inputs[0].destination, &outputs[0].format, &outputs[0], &inputs[0]);
						roster->GetFreeOutputsFor(filterNode, &outputs[1], 1, &numOutputs);
						roster->GetFreeInputsFor(writer->Node(), &inputs[1], 1, &numInputs);
						roster->Connect(outputs[1].source, inputs[1].destination, &inputs[1].format, &outputs[1], &inputs[1]);
						filterConnected = true;					
					}
					time = (bigtime_t*)timeList.ItemAt(timeEvent);
					time2 = (bigtime_t*)timeList.ItemAt(timeEvent + 1);
					
					/* Tell the nodes to start until next time event */
					roster->RollNode(writer->Node(), *time, *time2);
					if (filterConnected)
						roster->RollNode(filterNode, *time, *time2);
					if (transitionConnected)
						roster->RollNode(transitionNode, *time, *time2);
					if (reader2Connected)
						roster->RollNode(reader2->Node(), *time, *time2);
					if (reader1Connected)
						roster->RollNode(reader1->Node(), *time, *time2);
					break;
				case transition:
				
				/* Handle transition events */
					if (transitionConnected)
					{
						roster->GetConnectedOutputsFor(transitionNode, outputs, 1, &numOutputs);
						if (numOutputs == 1)
						{
							destinations[0] = outputs[0].destination;
							doConnect = true;
						}
						roster->GetConnectedInputsFor(transitionNode, inputs, 2, &numInputs);
						if (numInputs)
						{
							sources[0] = inputs[0].source;
							sources[1] = inputs[1].source;
						}
						roster->ReleaseNode(transitionNode);
						transitionConnected = false;
					}
					FindTransition(composant->u.transition.type, &transitionNode);
					roster->SetRunModeNode(transitionNode, BMediaNode::B_OFFLINE);
					if (doConnect)
					{
						roster->GetFreeInputsFor(transitionNode, &inputs[2], 2, &numInputs);
						roster->Connect(inputs[0].source, inputs[2].destination, &inputs[0].format, &outputs[1], &inputs[2]);
						roster->Connect(inputs[1].source, inputs[3].destination, &inputs[1].format, &outputs[1], &inputs[2]);
						roster->GetFreeOutputsFor(transitionNode, &outputs[1], 1, &numOutputs);
						roster->Connect(outputs[1].source, destinations[0], &outputs[0].format, &outputs[1], &inputs[0]);
						transitionConnected = true;
					}
					else if (filterConnected)
					{
						roster->GetConnectedInputsFor(filterNode, inputs, 1, &numInputs);
						if (reader1Connected)
							roster->Disconnect(reader1->Node().node, inputs[0].source, filterNode.node, inputs[0].destination);
						else if (reader2Connected)
							roster->Disconnect(reader2->Node().node, inputs[0].source, filterNode.node, inputs[0].destination);
						roster->GetFreeInputsFor(transitionNode, &inputs[2], 2, &numInputs);
						roster->GetFreeOutputsFor(reader1->Node(), &outputs[0], 1, &numOutputs);
						roster->Connect(outputs[0].source, inputs[2].destination, &outputs[0].format, &outputs[0], &inputs[2]);
						roster->GetFreeOutputsFor(reader2->Node(), &outputs[0], 1, &numOutputs);
						roster->Connect(outputs[0].source, inputs[3].destination, &outputs[0].format, &outputs[0], &inputs[3]);
						roster->GetFreeOutputsFor(transitionNode, &outputs[2], 1, &numOutputs);
						roster->Connect(outputs[2].source, inputs[0].destination, &inputs[0].format, &outputs[2], &inputs[0]);
						transitionConnected = true;
					}
					else if (reader1Connected && firstTrackActive)
					{
						roster->GetConnectedOutputsFor(reader1->Node(), &outputs[0], 1, &numOutputs);
						roster->Disconnect(reader1->Node().node, outputs[0].source, writer->Node().node, outputs[0].destination);
						roster->GetFreeInputsFor(transitionNode, &inputs[0], 2, &numInputs);
						roster->Connect(outputs[0].source, inputs[0].destination, &outputs[0].format, &outputs[0], &inputs[0]);
						roster->GetFreeOutputsFor(reader2->Node(), &outputs[1], 1, &numOutputs);
						roster->Connect(outputs[1].source, inputs[1].destination, &outputs[0].format, &outputs[1], &inputs[1]);
						roster->GetFreeOutputsFor(transitionNode, &outputs[1], 1, &numOutputs);
						roster->GetFreeInputsFor(writer->Node(), &inputs[1], 1, &numInputs);
						roster->Connect(outputs[1].source, inputs[1].destination, &inputs[1].format, &outputs[1], &inputs[1]);
						transitionConnected = true;
					}
					else if (reader2Connected && !firstTrackActive)
					{
						roster->GetConnectedOutputsFor(reader2->Node(), &outputs[0], 1, &numOutputs);
						roster->Disconnect(reader2->Node().node, outputs[0].source, writer->Node().node, outputs[0].destination);
						roster->GetFreeInputsFor(transitionNode, &inputs[0], 2, &numInputs);
						roster->Connect(outputs[0].source, inputs[0].destination, &outputs[0].format, &outputs[0], &inputs[0]);
						roster->GetFreeOutputsFor(reader1->Node(), &outputs[1], 1, &numOutputs);
						roster->Connect(outputs[1].source, inputs[1].destination, &outputs[0].format, &outputs[1], &inputs[1]);
						roster->GetFreeOutputsFor(transitionNode, &outputs[1], 1, &numOutputs);
						roster->GetFreeInputsFor(writer->Node(), &inputs[1], 1, &numInputs);
						roster->Connect(outputs[1].source, inputs[1].destination, &inputs[1].format, &outputs[1], &inputs[1]);
						transitionConnected = true;
					}
					time = (bigtime_t*)timeList.ItemAt(timeEvent);
					time2 = (bigtime_t*)timeList.ItemAt(timeEvent + 1);
					
					/* Tell the nodes to start until next time event */
					roster->RollNode(writer->Node(), *time, *time2);
					if (filterConnected)
						roster->RollNode(filterNode, *time, *time2);
					if (transitionConnected)
						roster->RollNode(transitionNode, *time, *time2);
					if (reader2Connected)
						roster->RollNode(reader2->Node(), *time, *time2);
					if (reader1Connected)
						roster->RollNode(reader1->Node(), *time, *time2);
					break;
				default:
					break;
			}
		}
	//	release_sem(lock_sem);
	}
	delete writer;
	return B_OK;
}

void VirtualRenderer::FindFilter(filter_type type, media_node *node)
{
	switch (type)
	{
		case blur:
			*node = (new BlurFilter)->Node();
			break;
		case black_white:
			*node = (new BWThresholdFilter)->Node();
			break;
		case contrast_brightness:
			*node = (new ContrastBrightnessFilter)->Node();
			break;
		case diff_detection:
			*node = (new DiffDetectionFilter)->Node();
			break;
		case emboss:
			*node = (new EmbossFilter)->Node();
			break;
		case frame_bin_op:
			*node = (new FrameBinOpFilter)->Node();
			break;
		case gray:
			*node = (new GrayFilter())->Node();
			break;
		case hv_mirror:
			*node = (new HVMirroringFilter)->Node();
			break;
		case invert:
			*node = (new InvertFilter)->Node();
			break;
		case levels:
			*node = (new LevelsFilter)->Node();
			break;
		case mix:
			*node = (new MixFilter)->Node();
			break;
		case motion_blur:
			*node = (new MotionBlurFilter)->Node();
			break;
		case motion_bw_threshold:
			*node = (new MotionBWThresholdFilter)->Node();
			break;
		case motion_rgb_threshold:
			*node = (new MotionRGBThresholdFilter)->Node();
			break;
		case motion_mask:
			*node = (new MotionMaskFilter)->Node();
			break;
		case mozaic:
			*node = (new MozaicFilter)->Node();
			break;
		case offset:
			*node = (new OffsetFilter)->Node();
			break;
		case rgb_intensity:
			*node = (new RGBIntensityFilter)->Node();
			break;
		case rgb_threshold:
			*node = (new RGBThresholdFilter)->Node();
			break;
		case solarize:
			*node = (new SolarizeFilter)->Node();
			break;
		case step_motion:
			*node = (new StepMotionBlurFilter)->Node();
			break;
		case trame:
			*node = (new TrameFilter)->Node();
			break;
		case rgb_channel:
			*node = (new ColorFilter)->Node();
			break;

	}
}

void VirtualRenderer::FindTransition(transition_type type, media_node * node)
{
	switch (type)
	{
		case cross_fader:
			*node = (new CrossFaderTransition)->Node();
			break;
		case disolve:
			*node = (new DisolveTransition)->Node();
			break;
		case flip:
			*node = (new FlipTransition)->Node();
			break;
		case gradient:
			*node = (new GradientTransition)->Node();
			break;
		case swap_transition:
			*node = (new SwapTransition)->Node();
			break;
		case venetian_stripes:
			*node = (new VenetianStripesTransition)->Node();
			break;
		case wipe:
			*node = (new WipeTransition)->Node();
			break;
	}
}

int32 VirtualRenderer::renderstart(void *castToVirtualRenderer)
{
	return ((VirtualRenderer*)castToVirtualRenderer)->RenderLoop();
}

void VirtualRenderer::SetFilterParameters(media_node node, parameter_list *list)
{
	BParameterWeb	*web;
	BParameterGroup	*group;
	BParameter		*param;
	int				i, j;
	parameter_list_elem	*elem;
	
	BMediaRoster::Roster()->GetParameterWebFor(node, &web);
	group = web->GroupAt(0);
	for (i = 0; i < group->CountParameters(); i++)
	{
		param = group->ParameterAt(i);
		for (j = 0; j < list->CountItems(); j++)
		{
			elem = list->ItemAt(j);
			if (elem->id == param->ID())
			{
				param->SetValue(elem->value, elem->value_size, system_time());
				break;
			}
		}
	}
}

void VirtualRenderer::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case B_MEDIA_NODE_STOPPED:
			release_sem(lock_sem);
//			acquire_sem(lock_sem);
			break;
		default:
			break;
	}
}











