#include <View.h>
#include <stdio.h>
#include <fcntl.h>
#include <Buffer.h>
#include <unistd.h>
#include <string.h>
#include <NodeInfo.h>
#include <scheduler.h>
#include <TimeSource.h>
#include <StringView.h>
#include <MediaRoster.h>
#include <Application.h>
#include <BufferGroup.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <Alert.h>

#include "DiskWriter.h"
#include "MediaUtils.h"

#define	FUNCTION	printf
#define ERROR		printf
#define PROGRESS	printf
#define LOOP		printf


#define ENCODED_FORMAT "avi"

const media_raw_video_format vid_format = { 29.97,1,0,239,B_VIDEO_TOP_LEFT_RIGHT,1,1,{B_RGB32,320,240,320*4,0,0}};

//---------------------------------------------------------------

DiskWriter::DiskWriter(entry_ref fileToWrite, media_file_format mfi, media_codec_info video_codec, media_codec_info audio_codec, const uint32 internal_id) :	
	BMediaNode("DiskWriter"),
	BMediaEventLooper(),
	BBufferConsumer(B_MEDIA_RAW_VIDEO),
	BFileInterface(),
	mInternalID(internal_id),
	mAddOn(NULL),
	mRate(1000000),
	mImageFormat(0),
	mTranslator(0),
	mMyLatency(20000),
	mBuffers(NULL),
	mOurBuffers(false),
	mFile(fileToWrite),
	mMediaFile(NULL)
{
	FUNCTION("DiskWriter::DiskWriter\n");
	
	mFileFormat = mfi;
	mVideoCodec = video_codec;
	mAudioCodec = audio_codec;
	AddNodeKind(B_FILE_INTERFACE);
	AddNodeKind(B_PHYSICAL_OUTPUT);
	SetEventLatency(mMyLatency);

	for (uint32 j = 0; j < 3; j++)
	{
		mBitmap[j] = NULL;
		mBufferMap[j] = 0;
	}
	SetPriority(B_REAL_TIME_PRIORITY);
	BMediaRoster::Roster()->RegisterNode(this);
	bigtime_t	bouh;
}

//---------------------------------------------------------------

DiskWriter::~DiskWriter()
{
	FUNCTION("DiskWriter::~DiskWriter\n");
	status_t status;
	
	if (mMediaFile)
		mMediaFile->CloseFile();

	Quit();

	DeleteBuffers();	
}

/********************************
	From BMediaNode
********************************/

//---------------------------------------------------------------

BMediaAddOn *
DiskWriter::AddOn(long *cookie) const
{
	FUNCTION("DiskWriter::AddOn\n");
	// do the right thing if we're ever used with an add-on
	*cookie = mInternalID;
	return mAddOn;
}

//---------------------------------------------------------------

// This implementation is required to get around a bug in
// the ppc compiler.

void 
DiskWriter::Start(bigtime_t performance_time)
{
	BMediaEventLooper::Start(performance_time);
	BMediaEventLooper::SetRunState(B_STARTED);
	if (RunMode() == B_OFFLINE)
		SetOfflineTime(performance_time);
}

void 
DiskWriter::Stop(bigtime_t performance_time, bool immediate)
{
	BMediaEventLooper::Stop(performance_time, immediate);
}

void 
DiskWriter::Seek(bigtime_t media_time, bigtime_t performance_time)
{
	BMediaEventLooper::Seek(media_time, performance_time);
}

void 
DiskWriter::TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time)
{
	BMediaEventLooper::TimeWarp(at_real_time, to_performance_time);
}

status_t 
DiskWriter::DeleteHook(BMediaNode *node)
{
	return BMediaEventLooper::DeleteHook(node);
}

//---------------------------------------------------------------

void
DiskWriter::NodeRegistered()
{
	FUNCTION("DiskWriter::NodeRegistered\n");
	mIn.destination.port = ControlPort();
	mIn.destination.id = 0;
	mIn.source = media_source::null;
	mIn.format.type = B_MEDIA_RAW_VIDEO;
//	mIn.format.u.raw_video = vid_format;
	

	Run();
}

//---------------------------------------------------------------

status_t
DiskWriter::RequestCompleted(const media_request_info & info)
{
	FUNCTION("DiskWriter::RequestCompleted\n");
	switch(info.what)
	{
		case media_request_info::B_SET_OUTPUT_BUFFERS_FOR:
			{
				if (info.status != B_OK)
					ERROR("DiskWriter::RequestCompleted: Not using our buffers!\n");
			}
			break;
		default: 
			break;
	}
	return B_OK;
}

//---------------------------------------------------------------

status_t
DiskWriter::HandleMessage(int32 message, const void * data, size_t size)
{
	FUNCTION("DiskWriter::HandleMessage\n");
	if ((BBufferConsumer::HandleMessage(message, data, size) < 0)
		&& (BMediaNode::HandleMessage(message, data, size) < 0))
	{
		HandleBadMessage(message, data, size);
		return B_ERROR;
	}
	return B_OK;
}


//---------------------------------------------------------------

void
DiskWriter::BufferReceived(BBuffer * buffer)
{
	LOOP("DiskWriter::Buffer #%d received\n", buffer->ID());

	if (RunState() == B_STOPPED)
	{
		buffer->Recycle();
		return;
	}
	if (RunMode() == B_OFFLINE)
		SetOfflineTime(buffer->Header()->start_time);
	media_timed_event event(buffer->Header()->start_time, BTimedEventQueue::B_HANDLE_BUFFER,
						buffer, BTimedEventQueue::B_RECYCLE_BUFFER);
	EventQueue()->AddEvent(event);
}


//---------------------------------------------------------------

void
DiskWriter::ProducerDataStatus(
	const media_destination &for_whom,
	int32 status,
	bigtime_t at_media_time)
{
	FUNCTION("DiskWriter::ProducerDataStatus\n");
	if (for_whom == mIn.destination)
	{
		media_timed_event event(at_media_time, BTimedEventQueue::B_DATA_STATUS,
			&mIn, BTimedEventQueue::B_NO_CLEANUP, status, 0, NULL);
		EventQueue()->AddEvent(event);
	}
}

//---------------------------------------------------------------

status_t
DiskWriter::CreateBuffers(
	const media_format & with_format)
{
	FUNCTION("DiskWriter::CreateBuffers\n");
	
	// delete any old buffers
	DeleteBuffers();	

	status_t status = B_OK;

	// create a buffer group
	uint32 mXSize = with_format.u.raw_video.display.line_width;
	uint32 mYSize = with_format.u.raw_video.display.line_count;	
	uint32 mRowBytes = with_format.u.raw_video.display.bytes_per_row;
	color_space mColorspace = with_format.u.raw_video.display.format;
	PROGRESS("DiskWriter::CreateBuffers - Colorspace = %d\n", mColorspace);

 	mBuffers = new BBufferGroup(4 * with_format.u.raw_video.display.line_width * with_format.u.raw_video.display.line_count, 4);
	status = mBuffers->InitCheck();
	if (B_OK != status)
	{
		ERROR("DiskWriter::CreateBuffers - ERROR CREATING BUFFER GROUP\n");
		return status;
	}
	// and attach the  bitmaps to the buffer group
	for (uint32 j=0; j < 3; j++)
	{
		mBitmap[j] = new BBitmap(BRect(0, 0, (mXSize-1), (mYSize - 1)), mColorspace, false, true);
		if (mBitmap[j]->IsValid())
		{						
			buffer_clone_info info;
			if ((info.area = area_for(mBitmap[j]->Bits())) == B_ERROR)
				ERROR("DiskWriter::CreateBuffers - ERROR IN AREA_FOR\n");;
			info.offset = 0;
			info.size = (size_t)mBitmap[j]->BitsLength();
			info.flags = j;
			info.buffer = 0;

			if ((status = mBuffers->AddBuffer(info)) != B_OK)
			{
				ERROR("DiskWriter::CreateBuffers - ERROR ADDING BUFFER TO GROUP\n");
				return status;
			} else PROGRESS("DiskWriter::CreateBuffers - SUCCESSFUL ADD BUFFER TO GROUP\n");
		}
		else 
		{
			ERROR("DiskWriter::CreateBuffers - ERROR CREATING VIDEO RING BUFFER: %08x\n", status);
			return B_ERROR;
		}	
	}
	
	BBuffer ** buffList = new BBuffer * [3];
	for (int j = 0; j < 3; j++) buffList[j] = 0;
	
	if ((status = mBuffers->GetBufferList(3, buffList)) == B_OK)					
		for (int j = 0; j < 3; j++)
			if (buffList[j] != NULL)
			{
				mBufferMap[j] = (uint32) buffList[j];
				PROGRESS(" j = %d buffer = %08x\n", j, mBufferMap[j]);
			}
			else
			{
				ERROR("DiskWriter::CreateBuffers ERROR MAPPING RING BUFFER\n");
				return B_ERROR;
			}
	else
		ERROR("DiskWriter::CreateBuffers ERROR IN GET BUFFER LIST\n");
		
	FUNCTION("DiskWriter::CreateBuffers - EXIT\n");
	return status;
}

//---------------------------------------------------------------

void
DiskWriter::DeleteBuffers()
{
	FUNCTION("DiskWriter::DeleteBuffers\n");
	status_t status;
	
	if (mBuffers)
	{
		delete mBuffers;
		mBuffers = NULL;
		
		for (uint32 j = 0; j < 3; j++)
			if (mBitmap[j]->IsValid())
			{
				delete mBitmap[j];
				mBitmap[j] = NULL;
			}
	}
	FUNCTION("DiskWriter::DeleteBuffers - EXIT\n");
}

//---------------------------------------------------------------

status_t
DiskWriter::Connected(
	const media_source & producer,
	const media_destination & where,
	const media_format & with_format,
	media_input * out_input)
{
	FUNCTION("DiskWriter::Connected\n");
	
	mIn.source = producer;
	mIn.format = with_format;
	mIn.node = Node();
	sprintf(mIn.name, "Video Test");
	*out_input = mIn;

	uint32 user_data = 0;
	int32 change_tag = 1;	
	if (CreateBuffers(with_format) == B_OK)
		BBufferConsumer::SetOutputBuffersFor(producer, mDestination, 
											mBuffers, (void *)&user_data, &change_tag, true);
	else
	{
		ERROR("DiskWriter::Connected - COULDN'T CREATE BUFFERS\n");
		return B_ERROR;
	}

	mConnectionActive = true;
		
	FUNCTION("DiskWriter::Connected - EXIT\n");
	return B_OK;
}

//---------------------------------------------------------------

void
DiskWriter::Disconnected(
	const media_source & producer,
	const media_destination & where)
{
	FUNCTION("DiskWriter::Disconnected\n");

	if (where == mIn.destination && producer == mIn.source)
	{
		// disconnect the connection
		mIn.source = media_source::null;
		mConnectionActive = false;
	}
//	if (mMediaFile)
//		mMediaFile->CloseFile();
	
		

}

//---------------------------------------------------------------

status_t
DiskWriter::AcceptFormat(const media_destination & dest,	media_format * format)
{
	FUNCTION("DiskWriter::AcceptFormat\n");
	
	if (dest != mIn.destination)
	{
		ERROR("DiskWriter::AcceptFormat - BAD DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;	
	}
	
	if (format->type == B_MEDIA_NO_TYPE)
		format->type = B_MEDIA_RAW_VIDEO;
	
	if (format->type != B_MEDIA_RAW_VIDEO)
	{
		ERROR("DiskWriter::AcceptFormat - BAD FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}

	if (format->u.raw_video.display.format != B_RGB32 &&
		format->u.raw_video.display.format != B_RGB16 &&
		format->u.raw_video.display.format != B_RGB15 &&			
		format->u.raw_video.display.format != B_GRAY8 &&			
		format->u.raw_video.display.format != media_raw_video_format::wildcard.display.format)
	{
		ERROR("AcceptFormat - not a format we know about!\n");
		return B_MEDIA_BAD_FORMAT;
	}
		
	if (format->u.raw_video.display.format == media_raw_video_format::wildcard.display.format)
	{
		format->u.raw_video.display.format = B_RGB32;
	}

	char format_string[256];		
	string_for_format(*format, format_string, 256);
	FUNCTION("DiskWriter::AcceptFormat: %s\n", format_string);

	return B_OK;
}

//---------------------------------------------------------------

status_t
DiskWriter::GetNextInput(
	int32 * cookie,
	media_input * out_input)
{
	FUNCTION("DiskWriter::GetNextInput\n");

	// custom build a destination for this connection
	// put connection number in id

	if (*cookie < 1)
	{
		mIn.node = Node();
		mIn.destination.id = *cookie;
		sprintf(mIn.name, "Video Test");
		*out_input = mIn;
		(*cookie)++;
		return B_OK;
	}
	else
	{
		ERROR("DiskWriter::GetNextInput - - BAD INDEX\n");
		return B_MEDIA_BAD_DESTINATION;
	}
}

//---------------------------------------------------------------

void
DiskWriter::DisposeInputCookie(int32 /*cookie*/)
{
}

//---------------------------------------------------------------

status_t
DiskWriter::GetLatencyFor(
	const media_destination &for_whom,
	bigtime_t * out_latency,
	media_node_id * out_timesource)
{
	FUNCTION("DiskWriter::GetLatencyFor\n");
	
	if (for_whom != mIn.destination)
		return B_MEDIA_BAD_DESTINATION;
	
	*out_latency = mMyLatency;
	*out_timesource = TimeSource()->ID();
	return B_OK;
}


//---------------------------------------------------------------

status_t
DiskWriter::FormatChanged(
				const media_source & producer,
				const media_destination & consumer, 
				int32 from_change_count,
				const media_format &format)
{
	FUNCTION("DiskWriter::FormatChanged\n");
	
	if (consumer != mIn.destination)
		return B_MEDIA_BAD_DESTINATION;

	if (producer != mIn.source)
		return B_MEDIA_BAD_SOURCE;

	mIn.format = format;
	
	return CreateBuffers(format);
}

//---------------------------------------------------------------

void
DiskWriter::HandleEvent(
	const media_timed_event *event,
	bigtime_t lateness,
	bool realTimeEvent)

{
	LOOP("DiskWriter::HandleEvent\n");
	
	BBuffer *buffer;
	media_timed_event	*ev;
	status_t	err;
	
	switch (event->type)
	{
		case BTimedEventQueue::B_START:
			PROGRESS("DiskWriter::HandleEvent - START\n");
			break;
		case BTimedEventQueue::B_STOP:
			PROGRESS("DiskWriter::HandleEvent - STOP\n");
			if (RunMode() == B_OFFLINE)
			{
				const media_timed_event	*ev = EventQueue()->FindFirstMatch(OfflineTime(), BTimedEventQueue::B_AFTER_TIME, true, BTimedEventQueue::B_START);
				if (ev)
					SetOfflineTime(ev->event_time);
				else // il n'y a pas de Start event...
				{
					EventQueue()->FlushEvents(event->event_time, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER);
					SetRunState(B_STOPPED);
				}
			}
			else
			{
				EventQueue()->FlushEvents(event->event_time, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER);
				SetRunState(B_STOPPED);
			}			
			break;
		case BTimedEventQueue::B_HANDLE_BUFFER:
			LOOP("DiskWriter::HandleEvent - HANDLE BUFFER\n");
			buffer = (BBuffer *) event->pointer;
			if ((RunState() == B_STARTED) && mConnectionActive)
			{
				WriteBuffer(buffer);
				if (mProducerDataStatus != B_DATA_AVAILABLE)
					printf("No data available, don't request buffers\n");
				if (RunMode() != B_OFFLINE)
					printf("Not in offline mode, don't request buffers\n");
				buffer->Recycle();
				if ((RunMode() == B_OFFLINE) && (mProducerDataStatus == B_DATA_AVAILABLE))
				{
					if ((err = RequestAdditionalBuffer(mIn.source, buffer)) == B_OK)
						printf("DiskWriter: Buffer Requested !\n");
					if (err)
						printf("Error while requesting additional buffers..\n");
				}
				
				if (mProducerDataStatus == B_PRODUCER_STOPPED)
				{
					media_timed_event	stopEvent(OfflineTime(), BTimedEventQueue::B_STOP);
					EventQueue()->AddEvent(stopEvent);
				}
			}
			else
			{
				printf("DiskWriter : Recycling buffer");
				buffer->Recycle();
			}
			break;
		case BTimedEventQueue::B_DATA_STATUS:
			PROGRESS("DiskWriter::HandleEvent - DATA_STATUS EVENT\n");
			mProducerDataStatus = event->data;
			break;
		case BTimedEventQueue::B_SEEK:
			PROGRESS("DiskWriter::HandleEvent - SEEK EVENT\n");
			SetOfflineTime(event->event_time);
			break;
		default:
			ERROR("DiskWriter::HandleEvent - BAD EVENT\n");
			break;
	}			
}

void DiskWriter::SetOfflineTime(bigtime_t time)
{
	BMediaEventLooper::SetOfflineTime(time);
}

bigtime_t DiskWriter::OfflineTime()
{
	return BMediaEventLooper::OfflineTime();
}

/* BFileInterface */
void DiskWriter::DisposeFileFormatCookie(int32 cookie)
{

}

status_t DiskWriter::GetDuration(bigtime_t *outDuration)
{
	return MediaDuration(mFile, outDuration);
}

status_t DiskWriter::GetNextFileFormat(int32 *cookie, media_file_format *outFormat)
{
	return get_next_file_format(cookie, outFormat);
}

status_t DiskWriter::GetRef(entry_ref *outRef, char *outMimeType)
{
	status_t	err;
	*outRef = mFile;
	BNode	fileNode(&mFile);
	if ((err = fileNode.InitCheck()))
		return err;
	BNodeInfo nodeInfo(&fileNode);
	nodeInfo.GetType(outMimeType);
	return err;
}

status_t DiskWriter::SetRef(const entry_ref &file, bool create, bigtime_t *outDuration)
{
	status_t err = B_OK;
	mFile = file;

	int32 cookie = 0;
	media_codec_info	mci;
	media_format		outfmt, format;
	media_raw_video_format *rvf(NULL);
	//err = MediaDuration(&mFile, outDuration);
	*outDuration = 0;
	if (err)
		return err;
	if (err == B_OK)
	{
		mMediaFile = new BMediaFile(&mFile, &mFileFormat, B_MEDIA_FILE_REPLACE_MODE);
		if ((err = mMediaFile->InitCheck()) != B_OK)
		{
			return err;
		}
		// construct desired decoded video format
		memset(&format, 0, sizeof(format));
		format.type = B_MEDIA_RAW_VIDEO;
		format.u.raw_video = vid_format;
//		mVidTrack = mMediaFile->CreateTrack(&mIn.format, &mVideoCodec);
		mVidTrack = mMediaFile->CreateTrack(&format, &mVideoCodec);
		if (mVidTrack == NULL)
			(new BAlert("Alert!", "N'a pas pu creer la piste video!", "OK"))->Go();
		else
			mMediaFile->CommitHeader();
	}
	
	return err;
}

status_t DiskWriter::SniffRef(const entry_ref &file, char *outMimeType, float *outQuality)
{
	status_t	err = B_OK;
	BNode	fileNode(&mFile);
	if ((err = fileNode.InitCheck()))
		return err;
	BNodeInfo nodeInfo(&fileNode);
	nodeInfo.GetType(outMimeType);
	*outQuality = 0.1;
	return err;
}


//---------------------------------------------------------------

void DiskWriter::WriteBuffer(BBuffer *buffer)
{
	printf("DiskWriter::WriteBuffer\n");
	if (!buffer)
	{
		printf("Buffer is NULL, bailing out..\n");
		return;
	}	
	media_header	*mh;
	char			*bufferData;
	status_t		err;
	mh = buffer->Header();
	bufferData = (char*)buffer->Data();
	
	printf("Writing buffer in track...\n");
//	err = mVidTrack->WriteChunk(bufferData, mh->size_used);
	err = mVidTrack->WriteFrames(bufferData, 1, mh->u.encoded_video.field_flags);
	if(err)
		printf("Problem while writing buffer\n");
	else
		printf("Buffer written successfully\n");
		
	//err = mVidTrack->WriteFrames(bufferData, 1);

}