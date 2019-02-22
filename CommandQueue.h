/*********************************************************************************
*  Copyright (c) 2015-2016, Danlu Guo
*                             University of Waterloo
*                
*  All rights reserved.
*********************************************************************************/

#ifndef COMMANDQUEUE_H
#define COMMANDQUEUE_H

#include <queue>
#include <map>
#include "BusPacket.h"

namespace DRAMController
{
	class CommandQueue
	{
	public:
		CommandQueue(bool perRequestor);
		virtual ~CommandQueue();
		
		bool isPerRequestor();

		// insert command based on either general criticality or requestorID
		bool insertCommand(BusPacket* command, bool critical);

		// check size of buffer
		unsigned int getRequestorIndex();
		unsigned int getRequestorSize(unsigned int index);
		unsigned int getSize(bool critical);

		// access the command on top of each command queue
		BusPacket* getRequestorCommand(unsigned int index);
		BusPacket* getCommand(bool critical);
		BusPacket* checkCommand(bool critical, unsigned int index);

		// remove most recently accessed command (from requestor queue or general fifo)
		void removeCommand(unsigned int requestorID);
		void removeCommand();
		
	private:
		// indicate if perRequestor buffer is enabled
		bool perRequestorEnable;
		// indicate the last accessed queue : critical or nonCritical
		bool scheduledQueue;
		bool requestorQueue;
		unsigned int requestorIndex;
		// order of requestorID in the buffer map; used to determine the requestor scheduling order and find in the bufferMap
		std::vector<unsigned int> requestorMap;
		// dynamic allocate buffer to individual requestors that share the same resource level
		std::map< unsigned int, std::vector<BusPacket*> > requestorBuffer;
		// general buffer for critical commands
		std::vector<BusPacket*> hrtBuffer;
		// general buffer for nonCritical commands
		std::vector<BusPacket*> srtBuffer;
	};
}
#endif

