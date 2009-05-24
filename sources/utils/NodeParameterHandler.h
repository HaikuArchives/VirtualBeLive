#ifndef NODE_PARAMETER_HANDLER
#define NODE_PARAMETER_HANDLER

#include <Controllable.h>
#include <Looper.h>
#include "AllNodes.h"

#include "EventList.h"

class NodeParameterHandler : public BLooper
{
public:
	NodeParameterHandler(EventComposant *composant);
	~NodeParameterHandler();
	
	virtual void MessageReceived(BMessage *message);
	static void CreateParameterList(parameter_list **list);
private:
	void			InstantiateNode();
	void			HandleMessage(BMessage *message);
//	BControllable	*node;
	BMediaNode		*node;
	EmbossFilter	*test;
	EventComposant	*fComposant;	
	BMediaRoster	*fRoster;
	BMessenger		*messenger;
};

#endif