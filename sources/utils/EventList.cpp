#include "EventList.h"

EventList::EventList():BList(1)
{

}

EventList::~EventList()
{

}

bool EventList::AddItem(EventComposant *item)
{
	int i;
	if (CountItems() == 0)
		return BList::AddItem(item);
		
	else
	{		
		for (i=0; i < BList::CountItems(); i++)
		{
			if (item->time <= ((EventComposant*)ItemAt(i))->time)	
				return BList::AddItem(item, i);
		}
		return BList::AddItem(item, i);
	}
}

EventComposant *EventList::ItemAt(int32 index) const
{
	return (EventComposant *)BList::ItemAt(index);
}

bool parameter_list::AddItem(parameter_list_elem *elem)
{
	int		i;
	bool	added = false;
	parameter_list_elem	*bouh;
	for (i = 0; i < CountItems(); i++)
	{
		if ((bouh = ItemAt(i))->id == elem->id)
		{
			memcpy(bouh->value, elem->value, bouh->value_size);
			added = true;
			delete elem;
		}
	}
	if (!added)
		return BList::AddItem(elem);
	return true;
}