/*********************************************************************************
*  Copyright (c) 2015-2016, Danlu Guo
*                             University of Waterloo
*                
*  All rights reserved.
*********************************************************************************/

#include <iostream>
#include "CommandQueue.h"
#include "global.h"
#include <algorithm>    // std::swap

using namespace DRAMController;

CommandQueue::CommandQueue(bool perRequestor)
{
	scheduledQueue = true;
	requestorQueue = false;
	requestorIndex = 0;
	perRequestorEnable = perRequestor;
}

CommandQueue::~CommandQueue()
{
	// std::queue<BusPacket*> empty;
	// std::swap(hrtBuffer, empty);
	// std::swap(srtBuffer, empty);
}

bool CommandQueue::isPerRequestor()
{
	return perRequestorEnable;
}

unsigned int CommandQueue::getRequestorIndex()
{
	return requestorMap.size();
}

unsigned int CommandQueue::getRequestorSize(unsigned int index) 
{
	if(index >= requestorMap.size()) {
		DEBUG("CQ 	: Accessed Queue is out of range");
		abort();
	}
	else {
		return requestorBuffer[requestorMap[index]].size();
	}
}

unsigned int CommandQueue::getSize(bool critical)
{
	if(critical) { 
		return hrtBuffer.size(); 
	}
	else { 
		return srtBuffer.size(); 
	}	
}

bool CommandQueue::insertCommand(BusPacket* command, bool critical)
{
	if(perRequestorEnable){
		if(requestorBuffer.find(command->requestorID) == requestorBuffer.end()) {
			// create the requestor buffer
			requestorBuffer[command->requestorID] = std::vector<BusPacket*>();
			// indicate the order of requestors in the requetor map
			requestorMap.push_back(command->requestorID);
		}
		requestorBuffer[command->requestorID].push_back(command);
	}
	else {
		if(critical) { 
			hrtBuffer.push_back(command); 
		}
		else {
			srtBuffer.push_back(command); 
		}		
	}
	return true;
}

BusPacket* CommandQueue::getCommand(bool critical)
{
	scheduledQueue = critical;
	if(critical && !hrtBuffer.empty()) { 
		return hrtBuffer.front(); 
	}
	else if(!critical && !srtBuffer.empty()) { 
		return srtBuffer.front(); 
	}
	else {
		DEBUG("CQ : check the size first getCmd");
		abort();
		return NULL; 
	}
}

BusPacket* CommandQueue::checkCommand(bool critical, unsigned int index)
{
	if(critical && !hrtBuffer.empty()) { 
		return hrtBuffer[index]; 
	}
	else if(!critical && !srtBuffer.empty()) { 
		return srtBuffer[index]; 
	}
	else {
		DEBUG("CQ : check the size first checkCmd");
		abort();
		return NULL; 
	}
}

BusPacket* CommandQueue::getRequestorCommand(unsigned int index)
{
	return requestorBuffer[requestorMap[index]].front();
}

void CommandQueue::removeCommand(unsigned int requestorID)
{
	requestorBuffer[requestorID].erase(requestorBuffer[requestorID].begin());
	// DEBUG("REMOVE REQ "<<requestorID);
}

void CommandQueue::removeCommand()
{
	if(scheduledQueue == true) {
		hrtBuffer.erase(hrtBuffer.begin());
	}
	else {
		srtBuffer.erase(srtBuffer.begin());
	}
}

