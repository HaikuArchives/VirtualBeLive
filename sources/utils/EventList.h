#ifndef EVENTLIST_H
#define EVENTLIST_H
#include <list.h>
#include <StorageKit.h>

enum EventType
{
	video1, video2, filter, transition
};


enum filter_type
{
	blur,
	black_white,
	contrast_brightness,
	diff_detection,
	emboss,
	frame_bin_op,
	gray,
	hv_mirror,
	invert,
	levels,
	mix,
	motion_blur,
	motion_bw_threshold,
	motion_rgb_threshold,
	motion_mask,
	mozaic,
	offset,
	rgb_intensity,
	rgb_threshold,
	solarize,
	step_motion,
	trame,
	rgb_channel
};

enum transition_type
{
	cross_fader,
	disolve,
	flip,
	gradient,
	swap_transition,
	venetian_stripes,
	wipe
};

struct video_event
{
	char		*filepath;
	bigtime_t	begin;
};

struct parameter_list_elem
{
	int32	id;
	void	*value;
	size_t	value_size;
};

class parameter_list : public BList
{
public:
	parameter_list() : BList(1){};
	~parameter_list(){};

	bool			AddItem(parameter_list_elem *elem);
	parameter_list_elem*	ItemAt(int32 index)
	{
		return (parameter_list_elem*)BList::ItemAt(index);
	};
};



struct filter_event
{
	filter_type		type;
	parameter_list	*param_list;
};

struct transition_event
{
	transition_type	type;
	parameter_list	*param_list;	
};

struct EventComposant
{
	EventType	event;
	char		*name;
	bigtime_t	time;
	bigtime_t	end;
	bool		select;
	bool		startRendered;
	bool		endRendered;
	union
	{
		video_event		video;
		filter_event	filter;
		transition_event transition;
	}u;
};

class EventList : public BList
{
	public:
		EventList();
		~EventList();
		
		virtual bool AddItem(EventComposant *item);
		virtual EventComposant *ItemAt(int32 index) const;
};

#endif