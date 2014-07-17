#include <MediaKit.h>
#include <Bitmap.h>

enum file_type
{
	VIDEO_FILE = 0,
	AUDIO_FILE,
	NOT_A_MEDIA_FILE
};

file_type ReadFirstPicture(entry_ref *ref, BBitmap **picture);
status_t MediaDuration(entry_ref file, bigtime_t *duration);