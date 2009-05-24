#ifndef VID_TEST_H
#define VID_TEST_H

#include <View.h>
#include <Bitmap.h>
#include <Window.h>
#include <MediaNode.h>
#include <TranslationKit.h>
#include <BufferConsumer.h>
#include <TimedEventQueue.h>
#include <MediaEventLooper.h>
#include <Entry.h>
#include <MediaEncoder.h>
#include <FileInterface.h>

class DiskWriter : 
	public BMediaEventLooper,
	public BBufferConsumer,
	public BFileInterface
{
public:
			DiskWriter(entry_ref fileToWrite, media_file_format mfi, media_codec_info video_codec, media_codec_info audio_codec, const uint32 internal_id);
			~DiskWriter();
	
/*	BMediaNode */
public:
	
	virtual	BMediaAddOn	*AddOn(long *cookie) const;
	
protected:

	virtual void		Start(bigtime_t performance_time);
	virtual void		Stop(bigtime_t performance_time, bool immediate);
	virtual void		Seek(bigtime_t media_time, bigtime_t performance_time);
	virtual void		TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time);

	virtual void		NodeRegistered();
	virtual	status_t 	RequestCompleted(
							const media_request_info & info);
							
	virtual	status_t 	HandleMessage(
							int32 message,
							const void * data,
							size_t size);

	virtual status_t	DeleteHook(BMediaNode * node);

/*  BMediaEventLooper */
protected:
	virtual void		HandleEvent(
							const media_timed_event *event,
							bigtime_t lateness,
							bool realTimeEvent);
	virtual bigtime_t	OfflineTime(void);
/*	BBufferConsumer */
public:
	
	virtual	status_t	AcceptFormat(
							const media_destination &dest,
							media_format * format);
	virtual	status_t	GetNextInput(
							int32 * cookie,
							media_input * out_input);
							
	virtual	void		DisposeInputCookie(
							int32 cookie);
	
protected:

	virtual	void		BufferReceived(
							BBuffer * buffer);
	
private:

	virtual	void		ProducerDataStatus(
							const media_destination &for_whom,
							int32 status,
							bigtime_t at_media_time);									
	virtual	status_t	GetLatencyFor(
							const media_destination &for_whom,
							bigtime_t * out_latency,
							media_node_id * out_id);	
	virtual	status_t	Connected(
							const media_source &producer,
							const media_destination &where,
							const media_format & with_format,
							media_input * out_input);							
	virtual	void		Disconnected(
							const media_source &producer,
							const media_destination &where);							
	virtual	status_t	FormatChanged(
							const media_source & producer,
							const media_destination & consumer, 
							int32 from_change_count,
							const media_format & format);
	virtual void		SetOfflineTime(bigtime_t time);						
/*	implementation */

protected:
/* BFileInterface */
	virtual void		DisposeFileFormatCookie(int32 cookie);
	virtual status_t	GetDuration(bigtime_t *outDuration);
	virtual status_t	GetNextFileFormat(int32 *cookie, media_file_format *outFormat);
	virtual status_t	GetRef(entry_ref *outRef, char *outMimeType);
	virtual status_t	SetRef(const entry_ref &file, bool create, bigtime_t *outDuration);
	virtual status_t	SniffRef(const entry_ref &file, char *outMimeType, float *outQuality);
public:
			status_t	CreateBuffers(const media_format & with_format);
			void		DeleteBuffers();
private:
	void					WriteBuffer(BBuffer *buffer);

	uint32					mInternalID;
	BMediaAddOn				*mAddOn;
	media_file_format		mFileFormat;
	media_codec_info		mVideoCodec;
	media_codec_info		mAudioCodec;

	bool					mConnectionActive;
	media_input				mIn;
	media_destination		mDestination;
	bigtime_t				mMyLatency;

	BBitmap					*mBitmap[3];
	bool					mOurBuffers;
	BBufferGroup			*mBuffers;
	uint32					mBufferMap[3];	
	
	bigtime_t				mRate;
	uint32					mImageFormat;
	int32					mTranslator;
	
	entry_ref				mFile;
	BMediaFile				*mMediaFile;
	BMediaTrack				*mVidTrack;
	int32 					mProducerDataStatus;
};

#endif
