#ifndef MEDIATRACKEXTRACTOR_H
#define MEDIATRACKEXTRACTOR_H


class MediaTrackExtractor : public BBufferProducer
{
public:
	MediaTrackExtractor(BMediaTrack *);
	~MediaTrackExtractor();
private:
	BBufferGroup	*BuffGroup;	
}

#endif