#include <fcntl.h>
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

#include <media/Buffer.h>
#include <media/BufferGroup.h>
#include <media/ParameterWeb.h>
#include <media/TimeSource.h>
#include <MediaRoster.h>

#include <support/Autolock.h>
#include <support/Debug.h>

#define TOUCH(x) ((void)(x))

#define PRINTF(a,b) \
		do { \
			if (a < 2) { \
				printf("FileReader::"); \
				printf b; \
			} \
		} while (0)

#include "FileReader.h"

#define FIELD_RATE 30.f


#define CHOOSE_STRING "Choose..."

FileReader::FileReader(const char *name, const char *filename, int32 internal_id)
  :	BMediaNode(name),
	BMediaEventLooper(),
	BBufferProducer(B_MEDIA_RAW_VIDEO),
	BControllable()
{
	fInitStatus = B_NO_INIT;

	fInternalID = internal_id;

	fBufferGroup = NULL;

	fThread = -1;
	fFrameSync = -1;
	fProcessingLatency = 0LL;

	fRunning = false;
	fConnected = false;
	fEnabled = false;

	fOutput.destination = media_destination::null;

	AddNodeKind(B_PHYSICAL_INPUT);

	fInitStatus = B_OK;
	fPath = (char*)malloc(256 * sizeof(char));
	strcpy(fPath, filename);
	
//	filePanel = new BFilePanel(B_OPEN_PANEL, NULL, NULL, 0, false, NULL, NULL, true, true);
	fRoster = BMediaRoster::Roster();
	fRoster->RegisterNode(this);
	return;
}

FileReader::~FileReader()
{
	if (fInitStatus == B_OK)
	{
		/* Clean up after ourselves, in case the application didn't make us
		 * do so. */
		if (fConnected)
			Disconnect(fOutput.source, fOutput.destination);
		if (fRunning)
			HandleStop();
	}
	Quit();
	if (mediaFile)
		mediaFile->CloseFile();
	fRoster->UnregisterNode(this);
	if (bitmap)
		delete bitmap;
	free(fPath);
/*	if (filePanel)
		delete filePanel;*/
}

/* BMediaNode */

port_id
FileReader::ControlPort() const
{
	return BMediaNode::ControlPort();
}

BMediaAddOn *
FileReader::AddOn(int32 *internal_id) const
{
	return NULL;
}

status_t 
FileReader::HandleMessage(int32 message, const void *data, size_t size)
{
	return B_ERROR;
}

void 
FileReader::Preroll()
{
	/* This hook may be called before the node is started to give the hardware
	 * a chance to start. */
}

void
FileReader::SetTimeSource(BTimeSource *time_source)
{
	/* Tell frame generation thread to recalculate delay value */
	release_sem(fFrameSync);
}

status_t
FileReader::RequestCompleted(const media_request_info &info)
{
	return BMediaNode::RequestCompleted(info);
}

/* BMediaEventLooper */

void 
FileReader::NodeRegistered()
{
	if (fInitStatus != B_OK) {
		ReportError(B_NODE_IN_DISTRESS);
		return;
	}

	/* Set up the parameter web */
/*	BParameterWeb *web = new BParameterWeb();
	BParameterGroup *main = web->MakeGroup(Name());
	BDiscreteParameter *files = main->MakeDiscreteBParameter(P_PATH, B_MEDIA_RAW_VIDEO, "File Path", "Path");
	files->AddItem(1, CHOOSE_STRING);
	files->AddItem(2, fPath);

	fLastPathChange = system_time();
*/
	/* After this call, the BControllable owns the BParameterWeb object and
	 * will delete it for you */
//	SetParameterWeb(web);

	fOutput.node = Node();
	fOutput.source.port = ControlPort();
	fOutput.source.id = 0;
	fOutput.destination = media_destination::null;
	strcpy(fOutput.name, Name());	

	/* MediaReader Code */
	err = get_ref_for_path(fPath, &ref);
	if (err) {
		printf("problem with get_ref_for_path() -- %s\n", strerror(err));
		exit(1);
	}

	// instantiate a BMediaFile object, and make sure there was no error.
	mediaFile = new BMediaFile(&ref);
	err = mediaFile->InitCheck();
	if (err) {
		printf("cannot contruct BMediaFile object -- %s\n", strerror(err));
		exit(1);
	}
	// count the tracks and instanciate them, one at a time

	numTracks = mediaFile->CountTracks();

	for(int i = 0; i < numTracks; i++)
	{
		track = mediaFile->TrackAt(i);
		if (!track) 
		{
			printf("cannot contruct BMediaTrack object\n");
			exit(1);
		}
		// get the encoded format
		
		err = track->EncodedFormat(&format);
		if (err) 
		{
			printf("BMediaTrack::EncodedFormat error -- %s\n", strerror(err));
			exit(1);
		}
		
		if (format.type == B_MEDIA_ENCODED_VIDEO) 
			break;
	}
	// allocate a bitmap large enough to contain the decoded frame.

	bounds.Set(0.0, 0.0, format.u.encoded_video.output.display.line_width - 1.0, 
			   format.u.encoded_video.output.display.line_count - 1.0);
	bitmap = new BBitmap(bounds, B_RGB32);

	
	/* Tailor these for the output of your device */
	fOutput.format.type = B_MEDIA_RAW_VIDEO;
	fOutput.format.u.raw_video = media_raw_video_format::wildcard;
	fOutput.format.u.raw_video.interlace = 1;
	fOutput.format.u.raw_video.display.format = bitmap->ColorSpace();
	fOutput.format.u.raw_video.display.line_width = (int32) bounds.Width() + 1;
	fOutput.format.u.raw_video.display.line_count = (int32) bounds.Height() + 1;

/*	fOutput.format.u.raw_video.last_active = (int32) (bounds.Height() - 1.0);
	fOutput.format.u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
	fOutput.format.u.raw_video.pixel_width_aspect = 1;
	fOutput.format.u.raw_video.pixel_height_aspect = 3;
	fOutput.format.u.raw_video.display.bytes_per_row = bitmap->BytesPerRow();*/
	err = track->DecodedFormat(&format);
	if (err) 
	{
		printf("error with BMediaTrack::DecodedFormat() -- %s\n", strerror(err));
	}


	/* Start the BMediaEventLooper control loop running */
	Run();
}

void
FileReader::Start(bigtime_t performance_time)
{
	BMediaEventLooper::Start(performance_time);
	printf("FileReader::Start()\n");
}

void
FileReader::Stop(bigtime_t performance_time, bool immediate)
{
	BMediaEventLooper::Stop(performance_time, immediate);
}

void
FileReader::Seek(bigtime_t media_time, bigtime_t performance_time)
{
	BMediaEventLooper::Seek(media_time, performance_time);
}

void
FileReader::TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time)
{
	BMediaEventLooper::TimeWarp(at_real_time, to_performance_time);
}

status_t
FileReader::AddTimer(bigtime_t at_performance_time, int32 cookie)
{
	return BMediaEventLooper::AddTimer(at_performance_time, cookie);
}

void
FileReader::SetRunMode(run_mode mode)
{
	BMediaEventLooper::SetRunMode(mode);
}

void 
FileReader::HandleEvent(const media_timed_event *event,
		bigtime_t lateness, bool realTimeEvent)
{
	TOUCH(lateness); TOUCH(realTimeEvent);
	bigtime_t dummy;
	switch(event->type)
	{
		case BTimedEventQueue::B_START:
			PRINTF(-1, ("HandleEvent: Start -- %Ld\n", event->event_time));
			if (RunMode() == B_OFFLINE)
				SendDataStatus(B_DATA_AVAILABLE, fOutput.destination, OfflineTime());
			else
				SendDataStatus(B_DATA_AVAILABLE, fOutput.destination, event->event_time);
			HandleStart(event->event_time);
			break;
		case BTimedEventQueue::B_STOP:
			HandleStop();
			if (RunMode() == B_OFFLINE)
				SendDataStatus(B_PRODUCER_STOPPED, fOutput.destination, OfflineTime());
			break;
		case BTimedEventQueue::B_WARP:
			HandleTimeWarp(event->bigdata);
			break;
		case BTimedEventQueue::B_SEEK:
			dummy = (bigtime_t)event->bigdata;
			track->SeekToTime(&dummy, B_MEDIA_SEEK_CLOSEST_BACKWARD);
			HandleSeek(event->bigdata);
			break;
		case BTimedEventQueue::B_HANDLE_BUFFER:
			break;
		case BTimedEventQueue::B_DATA_STATUS:
			SendDataStatus(mDataStatus, fOutput.destination, OfflineTime());
			break;

		case BTimedEventQueue::B_PARAMETER:
		default:
			PRINTF(-1, ("HandleEvent: Unhandled event -- %lx\n", event->type));
			break;
	}
}

void 
FileReader::CleanUpEvent(const media_timed_event *event)
{
	BMediaEventLooper::CleanUpEvent(event);
}

bigtime_t
FileReader::OfflineTime()
{
	return BMediaEventLooper::OfflineTime();
}

void
FileReader::ControlLoop()
{
	BMediaEventLooper::ControlLoop();
}

status_t
FileReader::DeleteHook(BMediaNode * node)
{
	return BMediaEventLooper::DeleteHook(node);
}

/* BBufferProducer */

status_t 
FileReader::FormatSuggestionRequested(
		media_type type, int32 quality, media_format *format)
{
	if (type != B_MEDIA_ENCODED_VIDEO)
		return B_MEDIA_BAD_FORMAT;

	TOUCH(quality);

	*format = fOutput.format;
	return B_OK;
}

status_t 
FileReader::FormatProposal(const media_source &output, media_format *format)
{
	status_t err;

	if (!format)
		return B_BAD_VALUE;

	if (output != fOutput.source)
		return B_MEDIA_BAD_SOURCE;

	err = format_is_compatible(*format, fOutput.format) ?
			B_OK : B_MEDIA_BAD_FORMAT;
	*format = fOutput.format;
	return err;
		
}

status_t 
FileReader::FormatChangeRequested(const media_source &source,
		const media_destination &destination, media_format *io_format,
		int32 *_deprecated_)
{
	TOUCH(destination); TOUCH(io_format); TOUCH(_deprecated_);
	PRINTF(-1, ("FormatChangeRequested()\n"));
	if (source != fOutput.source)
		return B_MEDIA_BAD_SOURCE;
	if (io_format->type != B_MEDIA_RAW_VIDEO)
		return B_ERROR;	
		
	fConnectedFormat = io_format->u.raw_video;
	track->DecodedFormat(io_format);
	return B_OK;
}

status_t 
FileReader::GetNextOutput(int32 *cookie, media_output *out_output)
{
/*	if (!out_output)
		return B_BAD_VALUE;
*/
	if ((*cookie) != 0)
		return B_BAD_INDEX;
	
	*out_output = fOutput;
	(*cookie)++;
	return B_OK;

}

status_t 
FileReader::DisposeOutputCookie(int32 cookie)
{
	TOUCH(cookie);

	return B_OK;
}

status_t 
FileReader::SetBufferGroup(const media_source &for_source,
		BBufferGroup *group)
{
	TOUCH(for_source); TOUCH(group);

	return B_ERROR;
}

status_t 
FileReader::VideoClippingChanged(const media_source &for_source,
		int16 num_shorts, int16 *clip_data,
		const media_video_display_info &display, int32 *_deprecated_)
{
	TOUCH(for_source); TOUCH(num_shorts); TOUCH(clip_data);
	TOUCH(display); TOUCH(_deprecated_);

	return B_ERROR;
}

status_t 
FileReader::GetLatency(bigtime_t *out_latency)
{
	*out_latency = EventLatency() + SchedulingLatency();
	return B_OK;
}

status_t 
FileReader::PrepareToConnect(const media_source &source,
		const media_destination &destination, media_format *format,
		media_source *out_source, char *out_name)
{
	PRINTF(1, ("PrepareToConnect() %ldx%ld\n", \
			format->u.raw_video.display.line_width, \
			format->u.raw_video.display.line_count));

	if (fConnected) {
		PRINTF(0, ("PrepareToConnect: Already connected\n"));
		return EALREADY;
	}

	if (source != fOutput.source)
		return B_MEDIA_BAD_SOURCE;
	
	if (fOutput.destination != media_destination::null)
		return B_MEDIA_ALREADY_CONNECTED;

	/* The format parameter comes in with the suggested format, and may be
	 * specialized as desired by the node */
	if (!format_is_compatible(*format, fOutput.format)) {
		*format = fOutput.format;
		return B_MEDIA_BAD_FORMAT;
	}

	if (format->u.raw_video.display.line_width == 0)
		format->u.raw_video.display.line_width = 320;
	if (format->u.raw_video.display.line_count == 0)
		format->u.raw_video.display.line_count = 240;
	if (format->u.raw_video.field_rate == 0)
		format->u.raw_video.field_rate = 29.97f;

	*out_source = fOutput.source;
	strcpy(out_name, fOutput.name);

	fOutput.destination = destination;

	return B_OK;
}

void 
FileReader::Connect(status_t error, const media_source &source,
		const media_destination &destination, const media_format &format,
		char *io_name)
{
	PRINTF(1, ("Connect() %ldx%ld\n", \
			format.u.raw_video.display.line_width, \
			format.u.raw_video.display.line_count));

	if (fConnected) {
		PRINTF(0, ("Connect: Already connected\n"));
		return;
	}

	if ((source != fOutput.source) || (error < B_OK) ||	!(&format)->Matches(&fOutput.format))
	{
		PRINTF(1, ("Connect: Connect error\n"));
		return;
	}

	fOutput.destination = destination;
	strcpy(io_name, fOutput.name);

	if (fOutput.format.u.raw_video.field_rate != 0.0f) {
		fPerformanceTimeBase = fPerformanceTimeBase +
				(bigtime_t)
					((fFrame - fFrameBase) *
					(1000000 / fOutput.format.u.raw_video.field_rate));
		fFrameBase = fFrame;
	}
	
	fConnectedFormat = format.u.raw_video;

	/* get the latency */
	bigtime_t latency = 0;
	media_node_id tsID = 0;
	FindLatencyFor(fOutput.destination, &latency, &tsID);
	#define NODE_LATENCY 1000
	SetEventLatency(latency + NODE_LATENCY);

	uint32 *buffer, *p;
	int64	fc;
	p = buffer = (uint32 *)malloc(4 * fConnectedFormat.display.line_count *
			fConnectedFormat.display.line_width);
	if (!buffer) {
		PRINTF(0, ("Connect: Out of memory\n"));
		return;
	}
	bigtime_t now = system_time();
	track->ReadFrames(p, &fc);
	fProcessingLatency = system_time() - now;
/*	if (buffer)
		free(buffer);*/
//	fc = 1;
//	track->SeekToFrame(&fc);

	/* Create the buffer group */
	fBufferGroup = new BBufferGroup(4 * fConnectedFormat.display.line_width *
			fConnectedFormat.display.line_count, 16);
	if (fBufferGroup->InitCheck() < B_OK) {
		delete fBufferGroup;
		fBufferGroup = NULL;
		return;
	}

	fConnected = true;
	fEnabled = true;

	/* Tell frame generation thread to recalculate delay value */
	release_sem(fFrameSync);
}

void 
FileReader::Disconnect(const media_source &source,
		const media_destination &destination)
{
	PRINTF(1, ("Disconnect()\n"));

	if (!fConnected) {
		PRINTF(0, ("Disconnect: Not connected\n"));
		return;
	}

	if ((source != fOutput.source) || (destination != fOutput.destination)) {
		PRINTF(0, ("Disconnect: Bad source and/or destination\n"));
		return;
	}

	fEnabled = false;
	fOutput.destination = media_destination::null;

	fLock.Lock();
		delete fBufferGroup;
		fBufferGroup = NULL;
	fLock.Unlock();

	fConnected = false;
}

void 
FileReader::LateNoticeReceived(const media_source &source,
		bigtime_t how_much, bigtime_t performance_time)
{
	TOUCH(source); TOUCH(how_much); TOUCH(performance_time);
}

void 
FileReader::EnableOutput(const media_source &source, bool enabled,
		int32 *_deprecated_)
{
	TOUCH(_deprecated_);

	if (source != fOutput.source)
		return;

	fEnabled = enabled;
}

status_t 
FileReader::SetPlayRate(int32 numer, int32 denom)
{
	TOUCH(numer); TOUCH(denom);

	return B_ERROR;
}

void 
FileReader::AdditionalBufferRequested(const media_source &source,
		media_buffer_id prev_buffer, bigtime_t prev_time,
		const media_seek_tag *prev_tag)
{
	TOUCH(source); TOUCH(prev_buffer); TOUCH(prev_time); TOUCH(prev_tag);
	status_t err;
	err = SendAdditionalBuffer();
}

void 
FileReader::LatencyChanged(const media_source &source,
		const media_destination &destination, bigtime_t new_latency,
		uint32 flags)
{
	TOUCH(source); TOUCH(destination); TOUCH(new_latency); TOUCH(flags);
}

/* BControllable */									

status_t 
FileReader::GetParameterValue(
	int32 id, bigtime_t *last_change, void *value, size_t *size)
{
	if (id != P_PATH)
		return B_BAD_VALUE;

	*last_change = fLastPathChange;
	*size = strlen(fPath) * sizeof(char);
	strcpy((char*)value, fPath);

	return B_OK;
}

void
FileReader::SetParameterValue(
	int32 id, bigtime_t when, const void *value, size_t size)
{
	if ((id != P_PATH) || !value )
		return;

	if (!strcmp(fPath, (char*)value))
		return;

	strcpy(fPath, (char*)value);
	fLastPathChange = when;
	BroadcastNewParameterValue(fLastPathChange, P_PATH, &fPath, strlen(fPath));
}

status_t
FileReader::StartControlPanel(BMessenger *out_messenger)
{
	return BControllable::StartControlPanel(out_messenger);
}

/* FileReader */

void
FileReader::HandleStart(bigtime_t performance_time)
{
	/* Start producing frames, even if the output hasn't been connected yet. */

	PRINTF(1, ("HandleStart(%Ld)\n", performance_time));

	if (fRunning) {
		PRINTF(-1, ("HandleStart: Node already started\n"));
		return;
	}

	fFrame = 0;
	fFrameBase = 0;
	fPerformanceTimeBase = performance_time;

	if (RunMode() == B_OFFLINE)
	{
		SetOfflineTime(performance_time);
		InitOfflineMode();
		SendAdditionalBuffer();
	}
	else
	{
		fFrameSync = create_sem(0, "frame synchronization");
		if (fFrameSync < B_OK)
			return;
	
		fThread = spawn_thread(_frame_generator_, "frame generator",
				B_NORMAL_PRIORITY, this);
		if (fThread < B_OK)
		{
			delete_sem(fFrameSync);
			return;
		}
		
		resume_thread(fThread);
	
		fRunning = true;
		return;
	}
	SetRunState(B_STARTED);
}

void
FileReader::HandleStop(void)
{
	PRINTF(1, ("HandleStop()\n"));

	if (!fRunning)
	{
		PRINTF(-1, ("HandleStop: Node isn't running\n"));
		return;
	}

	if (RunMode() == B_OFFLINE)
	{
		const media_timed_event	*ev = EventQueue()->FindFirstMatch(OfflineTime(), BTimedEventQueue::B_AFTER_TIME, true, BTimedEventQueue::B_START);
		if (ev)
			SetOfflineTime(ev->event_time);
		else // il n'y a pas de Start event...
		{
			EventQueue()->FlushEvents(OfflineTime(), BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER);
			SetRunState(B_STOPPED);
		}
	}
	else
	{
		delete_sem(fFrameSync);
		wait_for_thread(fThread, &fThread);
		int64	f = 1;
		track->SeekToFrame(&f);
	}	
	fRunning = false;
}

void
FileReader::HandleTimeWarp(bigtime_t performance_time)
{
	fPerformanceTimeBase = performance_time;
	fFrameBase = fFrame;

	/* Tell frame generation thread to recalculate delay value */
	release_sem(fFrameSync);
}

void
FileReader::HandleSeek(bigtime_t performance_time)
{
	fPerformanceTimeBase = performance_time;
	fFrameBase = fFrame;

	/* Tell frame generation thread to recalculate delay value */
	release_sem(fFrameSync);
}


void FileReader::InitOfflineMode()
{
	ff = 0;
	nbf = track->CountFrames();
}

status_t FileReader::SendAdditionalBuffer()
{
	/* Fetch a buffer from the buffer group */
	BBuffer *buffer = fBufferGroup->RequestBuffer(
					4 * fConnectedFormat.display.line_width *
					fConnectedFormat.display.line_count, 0LL);
	if (!buffer)
		return B_ERROR;

	fFrame++;
	/* Fill out the details about this buffer. */
	media_header *h = buffer->Header();
	h->type = B_MEDIA_RAW_VIDEO;
	h->size_used = 4 * fConnectedFormat.display.line_width *
					fConnectedFormat.display.line_count;
	/* For a buffer originating from a device, you might want to calculate
	 * this based on the PerformanceTimeFor the time your buffer arrived at
	 * the hardware (plus any applicable adjustments). */
	h->start_time = (bigtime_t)	((fFrame - fFrameBase) * (1000000 / fConnectedFormat.field_rate));
	h->file_pos = 0;
	h->orig_size = 0;
	h->data_offset = 0;
	h->u.raw_video.field_gamma = 1.0;
	h->u.raw_video.field_sequence = fFrame;
	h->u.raw_video.field_number = 0;
	h->u.raw_video.pulldown_number = 0;
	h->u.raw_video.first_active_line = 1;
	h->u.raw_video.line_count = fConnectedFormat.display.line_count;
	
	
	/* Fill in with video data */
	uint32 *p = (uint32 *)buffer->Data();
	track->ReadFrames(p, &fc);
	/* Send the buffer on down to the consumer */
	if (SendBuffer(buffer, fOutput.source, fOutput.destination) < B_OK)
	{
		PRINTF(-1, ("FrameGenerator: Error sending buffer\n"));
		/* If there is a problem sending the buffer, return it to its
		 * buffer group. */
		buffer->Recycle();
	};
	ff++;
	SetOfflineTime(h->start_time);
	
	return B_OK;
}

/* The following functions form the thread that generates frames. You should
 * replace this with the code that interfaces to your hardware. */
int32 
FileReader::FrameGenerator()
{
	bigtime_t wait_until = system_time();
	ff = 0;
	nbf = track->CountFrames();
	bool	go_on = true;
	while (go_on) 
	{
		status_t err = acquire_sem_etc(fFrameSync, 1, B_ABSOLUTE_TIMEOUT,
				wait_until);

		/* The only acceptable responses are B_OK and B_TIMED_OUT. Everything
		 * else means the thread should quit. Deleting the semaphore, as in
		 * FileReader::HandleStop(), will trigger this behavior. */
		if ((err != B_OK) && (err != B_TIMED_OUT))
			break;

		fFrame++;

		/* Recalculate the time until the thread should wake up to begin
		 * processing the next frame. Subtract fProcessingLatency so that
		 * the frame is sent in time. */
		wait_until = TimeSource()->RealTimeFor(fPerformanceTimeBase, 0) +
				(bigtime_t)
						((fFrame - fFrameBase) *
						(1000000 / fConnectedFormat.field_rate)) -
				fProcessingLatency;

		/* Drop frame if it's at least a frame late */
		if (wait_until < system_time())
			continue;

		/* If the semaphore was acquired successfully, it means something
		 * changed the timing information (see FileReader::Connect()) and
		 * so the thread should go back to sleep until the newly-calculated
		 * wait_until time. */
		if (err == B_OK)
			continue;

		/* Send buffers only if the node is running and the output has been
		 * enabled */
		if (!fRunning || !fEnabled)
			continue;

		BAutolock _(fLock);

		/* Fetch a buffer from the buffer group */
		BBuffer *buffer = fBufferGroup->RequestBuffer(
						4 * fConnectedFormat.display.line_width *
						fConnectedFormat.display.line_count, 0LL);
		if (!buffer)
			continue;

		/* Fill out the details about this buffer. */
		media_header *h = buffer->Header();
		h->type = B_MEDIA_RAW_VIDEO;
		h->time_source = TimeSource()->ID();
		h->size_used = 4 * fConnectedFormat.display.line_width *
						fConnectedFormat.display.line_count;
		/* For a buffer originating from a device, you might want to calculate
		 * this based on the PerformanceTimeFor the time your buffer arrived at
		 * the hardware (plus any applicable adjustments). */
		h->start_time = fPerformanceTimeBase +
						(bigtime_t)
							((fFrame - fFrameBase) *
							(1000000 / fConnectedFormat.field_rate));
		h->file_pos = 0;
		h->orig_size = 0;
		h->data_offset = 0;
		h->u.raw_video.field_gamma = 1.0;
		h->u.raw_video.field_sequence = fFrame;
		h->u.raw_video.field_number = 0;
		h->u.raw_video.pulldown_number = 0;
		h->u.raw_video.first_active_line = 1;
		h->u.raw_video.line_count = fConnectedFormat.display.line_count;


		/* Fill in with video data */
		uint32 *p = (uint32 *)buffer->Data();
		track->ReadFrames(p, &fc);
		/* Send the buffer on down to the consumer */
		if (SendBuffer(buffer, fOutput.source, fOutput.destination) < B_OK) {
			PRINTF(-1, ("FrameGenerator: Error sending buffer\n"));
			/* If there is a problem sending the buffer, return it to its
			 * buffer group. */
			buffer->Recycle();
		};
		ff++;
		if (ff >= nbf - 1)
		{
/*			ff = 1;
			track->SeekToFrame(&ff);
*/			go_on = false;
		}
	}

	return B_OK;
}

int32
FileReader::_frame_generator_(void *data)
{
	return ((FileReader *)data)->FrameGenerator();
}
