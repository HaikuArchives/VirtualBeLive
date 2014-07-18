#include "WipeTransition.h"

#include <Buffer.h>
#include <BufferGroup.h>
#include <ByteOrder.h>
#include <Debug.h>
#include <ParameterWeb.h>
#include <TimeSource.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#define	CALL		printf
#define	TIMING		printf
#define	INFO		printf
#define ERROR		printf

// -------------------------------------------------------- //
// constants 
// -------------------------------------------------------- //

// input-ID symbols
enum input_id_t {
	ID_1ST_VIDEO_INPUT,
	ID_2ND_VIDEO_INPUT
};
	
// output-ID symbols
enum output_id_t {
	ID_VIDEO_MIX_OUTPUT
};
	

const char* const WipeTransition::s_nodeName = "WipeTransition";

	
// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

WipeTransition::~WipeTransition() 
{
	// shut down
	if (buffers)
		delete buffers;
	Quit();
	fRoster->UnregisterNode(this);
}

WipeTransition::WipeTransition(BMediaAddOn* pAddOn) :
	// * init base classes
	BMediaNode(s_nodeName), // (virtual base)
	BBufferConsumer(B_MEDIA_RAW_VIDEO),
	BBufferProducer(B_MEDIA_RAW_VIDEO),
	BControllable(),
	BMediaEventLooper(),
	
	// * init connection state
	m_outputEnabled(true),
	m_downstreamLatency(0),
	m_processingLatency(0),
	
	// * init add-on stuff
	m_pAddOn(pAddOn)
{	
	// the rest of the initialization happens in NodeRegistered().
	BParameterWeb *web = new BParameterWeb();
	BParameterGroup *main = web->MakeGroup(Name());
	
	BContinuousParameter *pstate = main->MakeContinuousParameter(P_STATE,B_MEDIA_RAW_VIDEO,"State (%)","","",0.0,100.0,1.0);
	BDiscreteParameter *pmode = main->MakeDiscreteParameter(P_MODE,B_MEDIA_RAW_VIDEO,"Mode","");
		pmode->AddItem(0,"Right => Left"); 
		pmode->AddItem(1,"Left => Right"); 
		pmode->AddItem(2,"Top => Bottom"); 
		pmode->AddItem(3,"Bottom => Top"); 
	
	TMode = 0;
	TState = 0;
	
	fLastStateChange = system_time();

	firstInputBufferHere = false;
	secondInputBufferHere = false;
	buffers = NULL;
	transitionBuffer = NULL;
	/* After this call, the BControllable owns the BParameterWeb object and
	 * will delete it for you */
	SetParameterWeb(web);
	fRoster = BMediaRoster::Roster();
	fRoster->RegisterNode(this);
}
	
	
// -------------------------------------------------------- //
// *** BMediaNode
// -------------------------------------------------------- //

status_t WipeTransition::HandleMessage(
	int32 code,
	const void* pData,
	size_t size) {

	// pass off to each base class
	if(
		BBufferConsumer::HandleMessage(code, pData, size) &&
		BBufferProducer::HandleMessage(code, pData, size) &&
		BControllable::HandleMessage(code, pData, size) &&
		BMediaNode::HandleMessage(code, pData, size))
		BMediaNode::HandleBadMessage(code, pData, size);
	
	// +++++ return error on bad message?
	return B_OK;
}

BMediaAddOn* WipeTransition::AddOn(
	int32* poID) const {

	if(m_pAddOn)
		*poID = 0;
	return m_pAddOn;
}

void WipeTransition::SetRunMode(run_mode mode)
{
	// +++++ any other work to do?
	
	// hand off
	BMediaEventLooper::SetRunMode(mode);
}
	
// -------------------------------------------------------- //
// *** BMediaEventLooper
// -------------------------------------------------------- //

void WipeTransition::HandleEvent(const media_timed_event* pEvent, bigtime_t howLate, bool realTimeEvent)
{
	ASSERT(pEvent);
	
	switch(pEvent->type) {
//		case BTimedEventQueue::B_PARAMETER:
//			handleParameterEvent(pEvent);
//			break;
			
		case BTimedEventQueue::B_START:
			handleStartEvent(pEvent);
			break;
			
		case BTimedEventQueue::B_STOP:
			handleStopEvent(pEvent);
			break;
			
		default:
			ignoreEvent(pEvent);
			break;
	}
}

// "The Media Server calls this hook function after the node has
//  been registered.  This is derived from BMediaNode; BMediaEventLooper
//  implements it to call Run() automatically when the node is registered;
//  if you implement NodeRegistered() you should call through to
//  BMediaEventLooper::NodeRegistered() after you've done your custom 
//  operations."

void WipeTransition::NodeRegistered()
{
	PRINT(("WipeTransition::NodeRegistered()\n"));

	// Start the BMediaEventLooper thread
	SetPriority(B_REAL_TIME_PRIORITY);
	Run();

	// figure preferred ('template') format
	m_preferredFormat.type = B_MEDIA_RAW_VIDEO;
	m_preferredFormat.u.raw_video = media_raw_video_format::wildcard;
//	getPreferredFormat(m_preferredFormat);
	
	// initialize current format
	m_format.type = B_MEDIA_RAW_VIDEO;
	m_format.u.raw_video = media_raw_video_format::wildcard;
	
	// init input
	first_input.destination.port = ControlPort();
	first_input.destination.id = ID_1ST_VIDEO_INPUT;
	first_input.node = Node();
	first_input.source = media_source::null;
	first_input.format = m_format;
	strncpy(first_input.name, "First Video Input", B_MEDIA_NAME_LENGTH);

	second_input.destination.port = ControlPort();
	second_input.destination.id = ID_2ND_VIDEO_INPUT;
	second_input.node = Node();
	second_input.source = media_source::null;
	second_input.format = m_format;
	strncpy(second_input.name, "Second Video Input", B_MEDIA_NAME_LENGTH);

	
	// init output
	m_output.source.port = ControlPort();
	m_output.source.id = ID_VIDEO_MIX_OUTPUT;
	m_output.node = Node();
	m_output.destination = media_destination::null;
	m_output.format = m_format;
	strncpy(m_output.name, "Mix Output", B_MEDIA_NAME_LENGTH);
	
}
	
// "Augment OfflineTime() to compute the node's current time; it's called
//  by the Media Kit when it's in offline mode. Update any appropriate
//  internal information as well, then call through to the BMediaEventLooper
//  implementation."

bigtime_t WipeTransition::OfflineTime()
{
	return BMediaEventLooper::OfflineTime();
}

// -------------------------------------------------------- //
// *** BBufferConsumer
// -------------------------------------------------------- //

status_t WipeTransition::AcceptFormat(const media_destination& destination, media_format* format) 
{
	char format_string[256];
	string_for_format(*format, format_string, 256);
	CALL("ColorFilter::AcceptFormat(%s)\n", format_string);
/*
	check to see if the destination matches one of your input destinations 
		if not return B_MEDIA_BAD_DESTINATION 

	then check the format->type field and if set to B_MEDIA_NO_TYPE
		set it to your preferred type
	if format->type is not your preferred format type
		return B_MEDIA_BAD_FORMAT
	
	Step through the ENTIRE format structure.  For every field check to
	see if you support a filled in value, if not return B_MEDIA_BAD_FORMAT
	If it is a wildcard, fill it in with your preferred information
	If it is supported, move to the next field.
	If the format is supported, return B_OK
*/
	if ((destination == first_input.destination) || (destination == second_input.destination))
	{
		if (format->type == B_MEDIA_NO_TYPE)
			format->type = B_MEDIA_RAW_VIDEO;
		
		if (format->type != B_MEDIA_RAW_VIDEO)
			return B_MEDIA_BAD_FORMAT;
			
		/* start with the display info */
		/* color space - only 32 bit right now */
		if (format->u.raw_video.display.format == media_video_display_info::wildcard.format)
			format->u.raw_video.display.format = B_RGB32;
		if (format->u.raw_video.display.format != B_RGB32)
			return B_MEDIA_BAD_FORMAT;
		
		/* line width - if unspecified, use 320 */
		if (format->u.raw_video.display.line_width == media_video_display_info::wildcard.line_width)
			format->u.raw_video.display.line_width = 320;
		/* line count - if unspecified use 240 */
		if (format->u.raw_video.display.line_count == media_video_display_info::wildcard.line_count)
			format->u.raw_video.display.line_count = 240;

		/* bytes per row - needs to be 8bytes * line_width*/
		if (format->u.raw_video.display.bytes_per_row == media_video_display_info::wildcard.bytes_per_row)
			format->u.raw_video.display.bytes_per_row =
				(format->u.raw_video.display.line_width * 4);
		if (format->u.raw_video.display.bytes_per_row != (format->u.raw_video.display.line_width * 4)) {
			return B_MEDIA_BAD_FORMAT;
		}
		
		/* pixel offset */
		if (format->u.raw_video.display.pixel_offset == media_video_display_info::wildcard.pixel_offset)
			format->u.raw_video.display.pixel_offset = 0;
		if (format->u.raw_video.display.pixel_offset != 0)
			return B_MEDIA_BAD_FORMAT;
			
		/* line_offset */
		if (format->u.raw_video.display.line_offset == media_video_display_info::wildcard.line_offset)
			format->u.raw_video.display.line_offset = 0;
		if (format->u.raw_video.display.line_offset != 0)
			return B_MEDIA_BAD_FORMAT;

		/* field rate */
		if (format->u.raw_video.field_rate == media_raw_video_format::wildcard.field_rate)
			format->u.raw_video.field_rate = 30;	// 30 fps
		
		/* interlace */
		if (format->u.raw_video.interlace == media_raw_video_format::wildcard.interlace)
			format->u.raw_video.interlace = 1;
		if (format->u.raw_video.interlace != 1)
			return B_MEDIA_BAD_FORMAT;
		
		/* first active */
		if (format->u.raw_video.first_active == media_raw_video_format::wildcard.first_active)
			format->u.raw_video.first_active = 0;
		if (format->u.raw_video.first_active != 0)
			return B_MEDIA_BAD_FORMAT;
		
		/* last_active - needs to be display.line_count - 1*/
		if (format->u.raw_video.last_active == media_raw_video_format::wildcard.last_active)
			format->u.raw_video.last_active = format->u.raw_video.display.line_count - 1;
		if (format->u.raw_video.last_active != format->u.raw_video.display.line_count -1) {
			ERROR("last_active problem: %d %d\n", format->u.raw_video.last_active, format->u.raw_video.display.line_count - 1);
			return B_MEDIA_BAD_FORMAT;
		}	
		/* orientation */
		if (format->u.raw_video.orientation == media_raw_video_format::wildcard.orientation)
			format->u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
		if (format->u.raw_video.orientation != B_VIDEO_TOP_LEFT_RIGHT)
			return B_MEDIA_BAD_FORMAT;
			
		/* pixel width aspect */
		if (format->u.raw_video.pixel_width_aspect == media_raw_video_format::wildcard.pixel_width_aspect)
			format->u.raw_video.pixel_width_aspect = 1;
		if (format->u.raw_video.pixel_width_aspect != 1)
			return B_MEDIA_BAD_FORMAT;
			
		/* pixel height aspect */
		if (format->u.raw_video.pixel_height_aspect == media_raw_video_format::wildcard.pixel_height_aspect)
			format->u.raw_video.pixel_height_aspect = 1;
		if (format->u.raw_video.pixel_height_aspect != 1)
			return B_MEDIA_BAD_FORMAT;
			
		string_for_format(*format, format_string, 256);
		CALL("AcceptFormat OK!: %s\n", format_string);
		return B_OK;
	}
	else return B_MEDIA_BAD_DESTINATION;
}
	
// "If you're writing a node, and receive a buffer with the B_SMALL_BUFFER
//  flag set, you must recycle the buffer before returning."	

void WipeTransition::BufferReceived(BBuffer* pBuffer)
{
	ASSERT(pBuffer);

	// check buffer destination
	if ((pBuffer->Header()->destination != first_input.destination.id) && (pBuffer->Header()->destination != second_input.destination.id))
	{
		PRINT(("WipeTransition::BufferReceived():\n"
			"\tBad destination.\n"));
		pBuffer->Recycle();
		return;
	}
	
	if ((RunMode() != B_OFFLINE) && (pBuffer->Header()->time_source != TimeSource()->ID())) 
	{
		PRINT(("* timesource mismatch\n"));
	}

	// check output
	if (m_output.destination == media_destination::null || !m_outputEnabled) 
	{
		pBuffer->Recycle();
		return;
	}
	
	if (pBuffer->Header()->destination != first_input.destination.id)
	{// buffer vient de la premiere entree
		firstInputBufferHere = true;
		firstBuffer = pBuffer;
		PRINT(("First Buffer Received\n"));
	}
	else
	{// buffer vient de la 2eme entree
		secondInputBufferHere = true;
		secondBuffer = pBuffer;
		PRINT(("Second Buffer Received\n"));
	}
	
	if (firstInputBufferHere && secondInputBufferHere) // que ce passe-t-il si l'un des producteurs n'est plus valable ?
	{
		// process and retransmit buffer
		MakeTransition(firstBuffer, secondBuffer);		
	
		status_t err = SendBuffer(transitionBuffer, m_output.source, m_output.destination);
		if (err < B_OK)
		{
			PRINT(("WipeTransition::BufferReceived():\n"
				"\tSendBuffer() failed: %s\n", strerror(err)));
			transitionBuffer->Recycle();
		}
		firstBuffer->Recycle();
		secondBuffer->Recycle();
		firstInputBufferHere = false;
		secondInputBufferHere = false;

		if (RunMode() == B_OFFLINE)
		{
			SetOfflineTime(transitionBuffer->Header()->start_time);
//			RequestAdditionalBuffer(first_input.source, OfflineTime());
//			RequestAdditionalBuffer(second_input.source, OfflineTime());
		}
		// sent!
	}
}
	
// * make sure to fill in poInput->format with the contents of
//   pFormat; as of R4.5 the Media Kit passes poInput->format to
//   the producer in BBufferProducer::Connect().

status_t WipeTransition::Connected(
	const media_source& source,
	const media_destination& destination,
	const media_format& format,
	media_input* poInput) {
	
	PRINT(("WipeTransition::Connected()\n""\tto source %ld\n", source.id));
	
	// sanity check
	if ((destination != first_input.destination) && (destination != second_input.destination))
	{
		PRINT(("\tbad destination\n"));
		return B_MEDIA_BAD_DESTINATION;
	}
	if ((first_input.destination == destination) && (first_input.source != media_source::null))
	{
		PRINT(("\talready connected\n"));
		return B_MEDIA_ALREADY_CONNECTED;
	}

	if ((second_input.destination == destination) && (second_input.source != media_source::null))
	{
		PRINT(("\talready connected\n"));
		return B_MEDIA_ALREADY_CONNECTED;
	}

	if (first_input.destination  == destination)
	{
		// initialize input
		first_input.source = source;
		first_input.format = format;
		*poInput = first_input;
		// store format (this now constrains the output format)
		m_format = format;
		buffers = new BBufferGroup(m_format.u.raw_video.display.line_count * m_format.u.raw_video.display.line_width * 4, 16);
		if (buffers->InitCheck())
			return B_ERROR;
//		uint32 user_data = 0;
//		int32 change_tag = 1;
//		BBufferConsumer::SetOutputBuffersFor(source, destination, buffers, (void *)&user_data, &change_tag, true);
	}	
	if (second_input.destination  == destination)
	{
		// initialize input
		second_input.source = source;
		second_input.format = format;
		*poInput = second_input;
	}	

	return B_OK;
}

void WipeTransition::Disconnected(const media_source& source, const media_destination& destination)
{	
	PRINT(("WipeTransition::Disconnected()\n"));

	// sanity checks
	if ((first_input.source != source) && (second_input.source != source))
	{
		PRINT(("\tsource mismatch: expected ID %ld, got %ld\n",	first_input.source.id, source.id));
		PRINT(("\tsource mismatch: expected ID %ld, got %ld\n",	second_input.source.id, source.id));
		return;
	}
	if ((destination != first_input.destination) && (destination != second_input.destination))
	{
		PRINT(("\tdestination mismatch: expected ID %ld, got %ld\n", first_input.destination.id, destination.id));
		return;
	}

	if (destination == first_input.destination)
		// mark disconnected
		first_input.source = media_source::null;
	
	if (destination == second_input.destination)
		// mark disconnected
		second_input.source = media_source::null;
	
	// no output? clear format:
	if (m_output.destination == media_destination::null)
	{
		m_format.u.raw_video = media_raw_video_format::wildcard;
	}
	
	first_input.format = m_format;
	second_input.format = m_format;

}
		
void WipeTransition::DisposeInputCookie(int32 cookie)
{

}
	
// "You should implement this function so your node will know that the data
//  format is going to change. Note that this may be called in response to
//  your AcceptFormat() call, if your AcceptFormat() call alters any wildcard
//  fields in the specified format. 
//
//  Because FormatChanged() is called by the producer, you don't need to (and
//  shouldn't) ask it if the new format is acceptable. 
//
//  If the format change isn't possible, return an appropriate error from
//  FormatChanged(); this error will be passed back to the producer that
//  initiated the new format negotiation in the first place."

status_t WipeTransition::FormatChanged(
	const media_source& source,
	const media_destination& destination,
	int32 changeTag,
	const media_format& newFormat)
{
	
	// flat-out deny format changes
	return B_MEDIA_BAD_FORMAT;
}
		
status_t WipeTransition::GetLatencyFor(
	const media_destination& destination,
	bigtime_t* poLatency,
	media_node_id* poTimeSource) {
	
	PRINT(("WipeTransition::GetLatencyFor()\n"));
	
	// sanity check
	if(destination != first_input.destination) {
		PRINT(("\tbad destination\n"));
		return B_MEDIA_BAD_DESTINATION;
	}
	
	*poLatency = m_downstreamLatency + m_processingLatency;
	PRINT(("\treturning %Ld\n", *poLatency));
	*poTimeSource = TimeSource()->ID();
	return B_OK;
}
		
status_t WipeTransition::GetNextInput(int32* pioCookie, media_input* poInput)
{
	if (*pioCookie > 1)
		return B_BAD_INDEX;
	if (*pioCookie == 0)
		*poInput = first_input;
	else
		*poInput = second_input;

	++*pioCookie;
	return B_OK;
}

void WipeTransition::ProducerDataStatus(const media_destination& destination, int32 status, bigtime_t tpWhen)
{
	PRINT(("WipeTransition::ProducerDataStatus()\n"));
	
	// sanity check
	if (destination != first_input.destination)
	{
		PRINT(("\tbad destination\n"));
	}
	
	if (m_output.destination != media_destination::null)
	{
		// pass status downstream
		status_t err = SendDataStatus(status, m_output.destination, tpWhen);
		if (err < B_OK)
		{
			PRINT(("\tSendDataStatus(): %s\n", strerror(err)));
		}
	}
}
	
// "This function is provided to aid in supporting media formats in which the
//  outer encapsulation layer doesn't supply timing information. Producers will
//  tag the buffers they generate with seek tags; these tags can be used to
//  locate key frames in the media data."

status_t WipeTransition::SeekTagRequested(
	const media_destination& destination,
	bigtime_t targetTime,
	uint32 flags,
	media_seek_tag* poSeekTag,
	bigtime_t* poTaggedTime,
	uint32* poFlags)
{

	PRINT(("WipeTransition::SeekTagRequested()\n"
		"\tNot implemented.\n"));
	return B_ERROR;
}
	
// -------------------------------------------------------- //
// *** BBufferProducer
// -------------------------------------------------------- //

// "When a consumer calls BBufferConsumer::RequestAdditionalBuffer(), this
//  function is called as a result. Its job is to call SendBuffer() to
//  immediately send the next buffer to the consumer. The previousBufferID,
//  previousTime, and previousTag arguments identify the last buffer the
//  consumer received. Your node should respond by sending the next buffer
//  after the one described. 
//
//  The previousTag may be NULL.
//  Return B_OK if all is well; otherwise return an appropriate error code."

void WipeTransition::AdditionalBufferRequested(
	const media_source& source,
	media_buffer_id previousBufferID,
	bigtime_t previousTime,
	const media_seek_tag* pPreviousTag)
{
	RequestAdditionalBuffer(first_input.source, OfflineTime());
	RequestAdditionalBuffer(second_input.source, OfflineTime());
//	additionalbufferrequested = true;
	PRINT(("WipeTransition::AdditionalBufferRequested\n"));
}
		
void WipeTransition::Connect(status_t status, const media_source& source, const media_destination& destination, const media_format& format,	char* pioName)
{
	
	PRINT(("WipeTransition::Connect()\n"));
	status_t err;
	
	// connection failed?	
	if (status < B_OK)
	{
		PRINT(("\tStatus: %s\n", strerror(status)));
		// 'unreserve' the output
		m_output.destination = media_destination::null;
		return;
	}
	
	// connection established:
	strncpy(pioName, m_output.name, B_MEDIA_NAME_LENGTH);
	m_output.destination = destination;
	m_format = format;
	
	// figure downstream latency
	media_node_id timeSource;
	err = FindLatencyFor(m_output.destination, &m_downstreamLatency, &timeSource);
	if (err < B_OK)
	{
		PRINT(("\t!!! FindLatencyFor(): %s\n", strerror(err)));
	}
	PRINT(("\tdownstream latency = %Ld\n", m_downstreamLatency));
	
	// prepare the filter
	initFilter();

	// figure processing time
	m_processingLatency = calcProcessingLatency();
	PRINT(("\tprocessing latency = %Ld\n", m_processingLatency));
	
	// store summed latency
	SetEventLatency(m_downstreamLatency + m_processingLatency);
	
	if (first_input.source != media_source::null)
	{
		// pass new latency upstream
		err = SendLatencyChange(first_input.source,	first_input.destination, EventLatency() + SchedulingLatency());
		if (err < B_OK)
			PRINT(("\t!!! SendLatencyChange(): %s\n", strerror(err)));
	}
}
		
void WipeTransition::Disconnect(const media_source& source, const media_destination& destination)
{
	PRINT(("WipeTransition::Disconnect()\n"));
	
	// sanity checks	
	if (source != m_output.source)
	{
		PRINT(("\tbad source\n"));
		return;
	}
	if (destination != m_output.destination)
	{
		PRINT(("\tbad destination\n"));
		return;
	}
	
	// clean up
	m_output.destination = media_destination::null;
	
	// no input? clear format:
	if (first_input.source == media_source::null)
	{
		m_format.u.raw_video = media_raw_video_format::wildcard;
	}
	
	m_output.format = m_format;
	
	// +++++ other cleanup goes here
}
		
status_t WipeTransition::DisposeOutputCookie(int32 cookie)
{
	return B_OK;
}
		
void WipeTransition::EnableOutput(
	const media_source& source,
	bool enabled,
	int32* _deprecated_)
{
	PRINT(("WipeTransition::EnableOutput()\n"));
	if (source != m_output.source)
	{
		PRINT(("\tbad source\n"));
		return;
	}
	
	m_outputEnabled = enabled;
}
		
status_t WipeTransition::FormatChangeRequested(
	const media_source& source,
	const media_destination& destination,
	media_format* pioFormat,
	int32* _deprecated_)
{
	
	// deny
	PRINT(("WipeTransition::FormatChangeRequested()\n"
		"\tNot supported.\n"));
		
	return B_MEDIA_BAD_FORMAT;
}
		
status_t WipeTransition::FormatProposal(
	const media_source& source,
	media_format* pioFormat)
{

	PRINT(("WipeTransition::FormatProposal()\n"));
	
	if (source != m_output.source)
	{
		PRINT(("\tbad source\n"));
		return B_MEDIA_BAD_SOURCE;
	}
	
	if (pioFormat->type != B_MEDIA_RAW_VIDEO)
	{
		PRINT(("\tbad type\n"));
		return B_MEDIA_BAD_FORMAT;
	}
	*pioFormat = m_format;
	
	return B_OK;
}
	
status_t WipeTransition::FormatSuggestionRequested(
	media_type type,
	int32 quality,
	media_format* poFormat)
{	
	PRINT(("WipeTransition::FormatSuggestionRequested()\n"));
	if (type != B_MEDIA_RAW_VIDEO)
	{
		PRINT(("\tbad type\n"));
		return B_MEDIA_BAD_FORMAT;
	}
	
//	*poFormat = m_preferredFormat;
	*poFormat = m_format;
	return B_OK;
}
		
status_t WipeTransition::GetLatency(bigtime_t* poLatency)
{	
	PRINT(("WipeTransition::GetLatency()\n"));
	*poLatency = EventLatency() + SchedulingLatency();
	PRINT(("\treturning %Ld\n", *poLatency));
	
	return B_OK;
}
		
status_t WipeTransition::GetNextOutput(int32* pioCookie, media_output* poOutput)
{
	if (*pioCookie != 0)
		return B_BAD_INDEX;
	
	++*pioCookie;
	*poOutput = m_output;
	
	return B_OK;
}

// "This hook function is called when a BBufferConsumer that's receiving data
//  from you determines that its latency has changed. It will call its
//  BBufferConsumer::SendLatencyChange() function, and in response, the Media
//  Server will call your LatencyChanged() function.  The source argument
//  indicates your output that's involved in the connection, and destination
//  specifies the input on the consumer to which the connection is linked.
//  newLatency is the consumer's new latency. The flags are currently unused."
void WipeTransition::LatencyChanged(
	const media_source& source,
	const media_destination& destination,
	bigtime_t newLatency,
	uint32 flags)
{
	PRINT(("WipeTransition::LatencyChanged()\n"));
	
	if (source != m_output.source)
	{
		PRINT(("\tBad source.\n"));
		return;
	}
	if (destination != m_output.destination)
	{
		PRINT(("\tBad destination.\n"));
		return;
	}
	
	m_downstreamLatency = newLatency;
	SetEventLatency(m_downstreamLatency + m_processingLatency);
	
	if (first_input.source != media_source::null)
	{
		// pass new latency upstream
		status_t err = SendLatencyChange(first_input.source, first_input.destination, EventLatency() + SchedulingLatency());
		if(err < B_OK)
			PRINT(("\t!!! SendLatencyChange(): %s\n", strerror(err)));
	}
}

void WipeTransition::LateNoticeReceived(
	const media_source& source,
	bigtime_t howLate,
	bigtime_t tpWhen)
{
	PRINT(("WipeTransition::LateNoticeReceived()\n"
		"\thowLate == %Ld\n"
		"\twhen    == %Ld\n", howLate, tpWhen));
		
	if (source != m_output.source)
	{
		PRINT(("\tBad source.\n"));
		return;
	}

	if (first_input.source == media_source::null)
	{
		PRINT(("\t!!! No input to blame.\n"));
		return;
	}
	
	// +++++ check run mode?
	
	// pass the buck, since this node doesn't schedule buffer
	// production
	NotifyLateProducer(first_input.source, howLate,	tpWhen);
}
	
// PrepareToConnect() is the second stage of format negotiations that happens
// inside BMediaRoster::Connect().  At this point, the consumer's AcceptFormat()
// method has been called, and that node has potentially changed the proposed
// format.  It may also have left wildcards in the format.  PrepareToConnect()
// *must* fully specialize the format before returning!

status_t WipeTransition::PrepareToConnect(
	const media_source& source,
	const media_destination& destination,
	media_format* pioFormat,
	media_source* poSource,
	char* poName)
{
	char formatStr[256];
	string_for_format(*pioFormat, formatStr, 255);
	PRINT(("WipeTransition::PrepareToConnect()\n"
		"\tproposed format: %s\n", formatStr));
	
	if (source != m_output.source)
	{
		PRINT(("\tBad source.\n"));
		return B_MEDIA_BAD_SOURCE;
	}
	if (m_output.destination != media_destination::null)
	{
		PRINT(("\tAlready connected.\n"));
		return B_MEDIA_ALREADY_CONNECTED;
	}
	
	if (pioFormat->type != B_MEDIA_RAW_VIDEO)
	{
		PRINT(("\tBad format type.\n"));
		return B_MEDIA_BAD_FORMAT;
	}
	
	// fill in wildcards
//	specializeOutputFormat(*pioFormat);
	
	// reserve the output
	m_output.destination = destination;
	m_output.format = *pioFormat;
	
	// pass back source & output name
	*poSource = m_output.source;
	strncpy(poName, m_output.name, B_MEDIA_NAME_LENGTH);
	
	return B_OK;
}
		
status_t WipeTransition::SetBufferGroup(const media_source& source, BBufferGroup* pGroup)
{
	PRINT(("WipeTransition::SetBufferGroup()\n"));
	if (source != m_output.source)
	{
		PRINT(("\tBad source.\n"));
		return B_MEDIA_BAD_SOURCE;
	}
	
	if (first_input.source == media_source::null)
	{
		PRINT(("\tNo producer to send buffers to.\n"));
		return B_ERROR;
	}
	
	// +++++ is this right?  buffer-group selection gets
	//       all asynchronous and weird...
	int32 changeTag;
	return SetOutputBuffersFor(first_input.source, first_input.destination, pGroup,	0, &changeTag);
}

status_t WipeTransition::SetPlayRate(int32 numerator, int32 denominator)
{
	// not supported
	return B_ERROR;
}
	
status_t WipeTransition::VideoClippingChanged(
	const media_source& source,
	int16 numShorts,
	int16* pClipData,
	const media_video_display_info& display,
	int32* poFromChangeTag)
{
	// not sane
	return B_ERROR;
}

// -------------------------------------------------------- //
// *** BControllable
// -------------------------------------------------------- //

status_t WipeTransition::GetParameterValue(int32 id, bigtime_t* last_change, void* value, size_t* size)
{
	switch (id)
	{
		case P_STATE:
			*((float *)value) = TState;
			break;
			
		case P_MODE:
			*((float *)value) = TMode;
			break;
		
		default:
			return B_BAD_VALUE;
			break;
	}

	*last_change = fLastStateChange;
	*size = sizeof(float);

	return B_OK;
}
		
void WipeTransition::SetParameterValue(int32 id, bigtime_t when, const void *value,	size_t size)
{
	float	tmp;

	if (!value)
		return;

	switch (id)
	{
		case P_STATE:
			tmp = *((float*)value);
			TState = (uint32)tmp;
			BroadcastNewParameterValue(fLastStateChange, id, &TState, sizeof(uint32));
			break;
		
		case P_MODE:
			//tmp = *((float*)value);
			TMode = *(uint32*)value;
			BroadcastNewParameterValue(fLastStateChange, id, &TMode, sizeof(uint32));
			break;
				
		default:
			break;
	}
	fLastStateChange = when;
}

void WipeTransition::Start(bigtime_t performance_time)
{
	BMediaEventLooper::Start(performance_time);
}

void WipeTransition::Stop(bigtime_t performance_time, bool immediate)
{
	BMediaEventLooper::Stop(performance_time, immediate);
}

void WipeTransition::Seek(bigtime_t media_time, bigtime_t performance_time)
{
	BMediaEventLooper::Seek(media_time, performance_time);
}

void WipeTransition::TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time)
{
	BMediaEventLooper::TimeWarp(at_real_time, to_performance_time);
}


// -------------------------------------------------------- //
// *** HandleEvent() impl
// -------------------------------------------------------- //
	
void WipeTransition::handleStartEvent(
	const media_timed_event* pEvent) {
	PRINT(("WipeTransition::handleStartEvent\n"));

	startFilter();	
}
		
void WipeTransition::handleStopEvent(
	const media_timed_event* pEvent) {
	PRINT(("WipeTransition::handleStopEvent\n"));
	
	stopFilter();
}
		
void WipeTransition::ignoreEvent(const media_timed_event* pEvent)
{
	PRINT(("WipeTransition::ignoreEvent\n"));	
}
	

// -------------------------------------------------------- //
// *** internal operations
// -------------------------------------------------------- //


// fill in wildcards in the given format.
// (assumes the format passes validateProposedFormat().)
//void WipeTransition::specializeOutputFormat(media_format& ioFormat)
//{
//	char formatStr[256];
//	string_for_format(ioFormat, formatStr, 255);
//	PRINT(("WipeTransition::specializeOutputFormat()\n"
//		"\tinput format: %s\n", formatStr));
//	
//	ASSERT(ioFormat.type == B_MEDIA_RAW_VIDEO);
//	
//	// carpal_tunnel_paranoia
//	media_raw_video_format& f = ioFormat.u.raw_video;
//	media_raw_video_format& w = media_raw_video_format::wildcard;
//	
//	if(f.frame_rate == w.frame_rate)
//		f.frame_rate = 44100.0;
//	if(f.channel_count == w.channel_count) {
//		//+++++ tweaked 15sep99
//		if(first_input.source != media_source::null)
//			f.channel_count = first_input.format.u.raw_video.channel_count;
//		else
//			f.channel_count = 1; 
//	}
//	if(f.format == w.format)
//		f.display.format = RGB_32;
//	if(f.orientation == w.format)
//		f.orientation = B_VIDEO_TOP_LEFT_RIGHT;
//
//	string_for_format(ioFormat, formatStr, 255);
//	PRINT(("\toutput format: %s\n", formatStr));
//}



// construct delay line if necessary, reset filter state
void WipeTransition::initFilter() {
	PRINT(("WipeTransition::initFilter()\n"));
	ASSERT(m_format.u.raw_video.format != media_raw_video_format::wildcard.format);
		
	m_framesSent = 0;
	m_delayWriteFrame = 0;
}

void WipeTransition::startFilter() {
	PRINT(("WipeTransition::startFilter()\n"));
}
void WipeTransition::stopFilter() {
	PRINT(("WipeTransition::stopFilter()\n"));
}

// figure processing latency by doing 'dry runs' of filterBuffer()
bigtime_t WipeTransition::calcProcessingLatency() {
	PRINT(("WipeTransition::calcProcessingLatency()\n"));
	
	if(m_output.destination == media_destination::null) {
		PRINT(("\tNot connected.\n"));
		return 0LL;
	}
	
	// allocate a temporary buffer group
	BBufferGroup* pTestGroup = new BBufferGroup(4 * m_output.format.u.raw_video.display.line_width * m_output.format.u.raw_video.display.line_count, 2);
	
	// fetch a buffer
	BBuffer* pBuffer = pTestGroup->RequestBuffer(4 * m_output.format.u.raw_video.display.line_width * m_output.format.u.raw_video.display.line_count);
	BBuffer* pBuffer2 = pTestGroup->RequestBuffer(4 * m_output.format.u.raw_video.display.line_width * m_output.format.u.raw_video.display.line_count);
	ASSERT(pBuffer);
	ASSERT(pBuffer2);
	
	pBuffer->Header()->type = B_MEDIA_RAW_VIDEO;
	pBuffer->Header()->size_used = m_output.format.u.raw_video.display.line_width * m_output.format.u.raw_video.display.line_count;

	pBuffer2->Header()->type = B_MEDIA_RAW_VIDEO;
	pBuffer2->Header()->size_used = m_output.format.u.raw_video.display.line_width * m_output.format.u.raw_video.display.line_count;
	
	// run the test
	bigtime_t preTest = system_time();
	MakeTransition(pBuffer, pBuffer2);
	bigtime_t elapsed = system_time()-preTest;
	
	// clean up
	pBuffer->Recycle();
	pBuffer2->Recycle();
	delete pTestGroup;

	// reset filter state
	initFilter();

	return elapsed;
}


void WipeTransition::MakeTransition(BBuffer* inBuffer, BBuffer *inBuffer2)
{
	status_t	err;
	if (!inBuffer || !inBuffer2)
		return;
	/* here is where we do all of the real work */
	if (RunMode() != B_OFFLINE)
		CALL("FilterBuffer now: %Ld\n", TimeSource()->Now());
	else
		CALL("FilterBuffer now: %Ld\n", OfflineTime());

	media_header *firstHeader = inBuffer->Header();
	media_header *secondHeader = inBuffer2->Header();

//	CALL("now: %Ld start_time: %Ld\n", TimeSource()->Now(), inHeader->start_time);
	transitionBuffer = buffers->RequestBuffer(4 * m_format.u.raw_video.display.line_width * m_format.u.raw_video.display.line_count, 10000);
	if (transitionBuffer == NULL)
	{
		err = buffers->RequestError();
		if (err == B_ERROR)
			printf("Error requesting buffer\n");
		else if (err == B_MEDIA_BUFFERS_NOT_RECLAIMED)
			printf("Buffers not reclaimed");
	}
	else
	{
		uint32 *inData1 = (uint32*) inBuffer->Data();
		uint32 *inData2 = (uint32*) inBuffer2->Data();
		uint32 *finalData = (uint32*) transitionBuffer->Data();
		
	/* WORK IT OUT  */
	
		uint32 * p1 = (uint32*)inData1;
		uint32 * p2 = (uint32*)inData2;
		uint32 * po = (uint32*)finalData;
		
		uint32 c,l,delta;
		
		const unsigned int COL=m_format.u.raw_video.display.line_width;
		const unsigned int LIN=m_format.u.raw_video.display.line_count;
		
		
		switch(TMode){
			case 0:
				delta = TState * COL / 100;
				for (l=0;l<LIN;l++)
					for (c=0;c<COL;c++){
						if ((c+delta)<COL) *po++ = p1[l*COL+c+delta];
						else *po++ = p2[l*COL+c+delta-COL];
					}
				break;
				
			case 1:
				delta = TState * COL / 100;
				for (l=0;l<LIN;l++)
					for (c=0;c<COL;c++){
						if ((c-delta)>=0) *po++ = p1[l*COL+c-delta];
						else *po++ = p2[l*COL+c-delta+COL];
					}
				break;
				
			case 2:
				delta = TState * LIN / 100;
				for (l=0;l<LIN;l++)
					for (c=0;c<COL;c++){
						if ((l+delta)<LIN) *po++ = p1[(l+delta)*COL+c];
						else *po++ = p2[(l+delta-LIN)*COL+c];
					}
				break;
				
			case 3:
				delta = TState * LIN / 100;
				for (l=0;l<LIN;l++)
					for (c=0;c<COL;c++){
						if ((l-delta)>=0) *po++ = p1[(l-delta)*COL+c];
						else *po++ = p2[(l-delta+LIN)*COL+c];
					}
				break;
				
			default:
				break;
				
		}
		
		// AUTO ++
		
		if (TState<100) TState++;
			
	// GO HOME!
	
		media_header *h = transitionBuffer->Header();
		h->type = B_MEDIA_RAW_VIDEO;
		h->size_used = firstHeader->size_used;
		h->start_time = firstHeader->start_time; // A changer !! IMPORTANT
		h->file_pos = firstHeader->file_pos;
		memcpy(&h->u.raw_video, &m_format.u.raw_video, sizeof(media_video_header));
//		memcpy(h, firstHeader, sizeof(media_header));
	}
// Fin Sans Bitmap	
}

// END -- WipeTransition.cpp --
