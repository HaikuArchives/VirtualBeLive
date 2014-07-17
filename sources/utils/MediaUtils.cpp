#include "MediaUtils.h"
#include <stdio.h>

file_type ReadFirstPicture(entry_ref *ref, BBitmap **picture)
{
	status_t		err;
	media_format	format;
	BMediaFile		mediaFile(ref);
	BMediaTrack		*track;
	int32			numTracks;
	int32			i;
	int64			j;
	int64			frameCount;
	BRect			bounds;
	char			*buffer;
	media_header	mh;
	BBitmap			*bitmap;
	if ((err = mediaFile.InitCheck()) != B_OK)
	{
		mediaFile.CloseFile();
		return NOT_A_MEDIA_FILE;
	}
	
	numTracks = mediaFile.CountTracks();
	if (numTracks == 0)
	{
		mediaFile.CloseFile();
		return NOT_A_MEDIA_FILE;
	}
	
	for (i = 0; i < numTracks; i++) 
	{
		track = mediaFile.TrackAt(i);
		if (!track) 
		{
			printf("cannot contruct BMediaTrack object\n");
			mediaFile.CloseFile();
			return NOT_A_MEDIA_FILE;
		}

		// get the encoded format
	
		err = track->EncodedFormat(&format);
		if (err) 
		{
			printf("BMediaTrack::EncodedFormat error -- %s\n", strerror(err));
			mediaFile.CloseFile();
			return NOT_A_MEDIA_FILE;
		}
	
		switch(format.type) 
		{
			// handle the case of an encoded video track
		
		 	case B_MEDIA_ENCODED_VIDEO:
		
				// allocate a bitmap large enough to contain the decoded frame.
		
				bounds.Set(0.0, 0.0, format.u.encoded_video.output.display.line_width - 1.0, 
						   format.u.encoded_video.output.display.line_count - 1.0);
				bitmap = new BBitmap(bounds, B_RGB32);
		
				// specifiy the decoded format. we derive this information from
				// the encoded format.
		
				memset(&format, 0, sizeof(media_format));
				format.u.raw_video.last_active = (int32) (bounds.Height() - 1.0);
				format.u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
				format.u.raw_video.pixel_width_aspect = 1;
				format.u.raw_video.pixel_height_aspect = 3;
				format.u.raw_video.display.format = bitmap->ColorSpace();
				format.u.raw_video.display.line_width = (int32) bounds.Width();
				format.u.raw_video.display.line_count = (int32) bounds.Height();
				format.u.raw_video.display.bytes_per_row = bitmap->BytesPerRow();
				err = track->DecodedFormat(&format);
				if (err) 
				{
					printf("error with BMediaTrack::DecodedFormat() -- %s\n", strerror(err));
					mediaFile.CloseFile();
					return NOT_A_MEDIA_FILE;
				}
		
				// iterate through all the frames and call the processing function
		
				buffer = (char *)bitmap->Bits();
				err = track->ReadFrames(buffer, &frameCount, &mh);
				if (err) 
				{
					printf("BMediaTrack::ReadFrames error -- %s\n", strerror(err));
					mediaFile.CloseFile();
					return NOT_A_MEDIA_FILE;
				}
				*picture = bitmap;
				mediaFile.CloseFile();
				return VIDEO_FILE;
				break;
				
			default:
				break;
		}
	}
	mediaFile.CloseFile();
	return AUDIO_FILE;
}

status_t MediaDuration(entry_ref file, bigtime_t *end)
{
	status_t		err;
	BMediaFile		mediaFile(&file);
	BMediaTrack		*track;
	int32			numTracks;
	int32			i;
	bigtime_t		maxDuration;
	
	maxDuration = 0;
	*end = 0;
	if ((err = mediaFile.InitCheck()) != B_OK)
		return err;
	numTracks = mediaFile.CountTracks();
	if (numTracks <= 0)
		return B_ERROR;
	for (i = 0; i < numTracks; i++) 
	{
		track = mediaFile.TrackAt(i);
		if (!track) 
		{
			printf("cannot contruct BMediaTrack object\n");
			return B_ERROR;
		}
		if (track->Duration() > maxDuration)
			maxDuration = track->Duration();
	}
	*end = maxDuration;
	mediaFile.CloseFile();
	return B_OK;
}