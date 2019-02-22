
#include "RequestQueue.h"

using namespace DRAMController;

#define DEBUG(str) std::cerr<< str <<std::endl;

RequestQueue::RequestQueue(bool perRequestor, bool writeQueueEnable):
	writeQueueEnable(writeQueueEnable),
	perRequestorEnable(perRequestor)
{
	if(writeQueueEnable) {
		writeQueue = new WriteQueue(16,32);
	}else {
		writeQueue = NULL;
	}
	generalBuffer = std::vector< Request* >();
	requestorBuffer = std::map<unsigned int, std::vector<Request*>>();
}

RequestQueue::~RequestQueue()
{
	delete writeQueue;
	for(auto it=generalBuffer.begin(); it!=generalBuffer.end(); it++) {
		delete (*it);
	}
	generalBuffer.clear();
	for(auto it=requestorBuffer.begin(); it!=requestorBuffer.end(); it++) {
		for(auto req=it->second.begin(); req!=it->second.end(); req++) {
			delete (*req);
		}
		it->second.clear();
	}
	requestorBuffer.clear();
}

bool RequestQueue::isWriteEnable()
{
	return writeQueueEnable;
}

bool RequestQueue::isPerRequestor()
{
	return perRequestorEnable;
}

// add request based on criticality
bool RequestQueue::insertRequest(Request* request)
{
	if(writeQueueEnable && request->requestType == DATA_WRITE) {
		writeQueue->insertWrite(request);
		return true;
	}

	if(perRequestorEnable) {
		if(requestorBuffer.find(request->requestorID) == requestorBuffer.end()) {
			requestorBuffer[request->requestorID] = std::vector<Request*>();
			requestorOrder.push_back(request->requestorID);
		}
		requestorBuffer[request->requestorID].push_back(request);
	}
	else {
		generalBuffer.push_back(request);
	}
	return true;
}

unsigned int RequestQueue::getQueueSize()
{
	// How many requestor share the same queue
	return requestorOrder.size();
}

unsigned int RequestQueue::getSize(bool requestor, unsigned int index)
{
	if(requestor) {
		if(requestorOrder.size() == 0) {
			return 0; 
		}
		else {
			// no such requestor
			if(requestorBuffer.find(requestorOrder[index]) == requestorBuffer.end()){ 
				return 0; 
			}	
			else { 
				return requestorBuffer[requestorOrder[index]].size();
			}
		}
	}
	else { 
		return generalBuffer.size();
	}
}

// If care about the fairness
Request* RequestQueue::getRequest(unsigned int reqIndex, unsigned int index)
{
	// scan the requestorqueue by index, instead of requestorID value 
	if(requestorBuffer[requestorOrder[reqIndex]].empty()) {
		return NULL;
	}
	else {
		scheduledRequest.first = true;
		scheduledRequest.second = std::make_pair(requestorOrder[reqIndex],index);
		return requestorBuffer[requestorOrder[reqIndex]][index];
	}
}

// If does not care about the fairness
Request* RequestQueue::getRequest(unsigned int index)
{
	if(generalBuffer.empty()) {
		return NULL;
	}
	else {
		scheduledRequest.first = false;
		scheduledRequest.second = std::make_pair(0,index);
		return generalBuffer[index];		
	}
}

// Remove the previously access request, Once a request is buffered to another location, remove 
// from the request queue
void RequestQueue::removeRequest()
{
	unsigned id = scheduledRequest.second.first;
	unsigned index = scheduledRequest.second.second;
	if(scheduledRequest.first) {
		requestorBuffer[id].erase(requestorBuffer[id].begin() + index);
	}
	else {
		generalBuffer.erase(generalBuffer.begin() + index);
	}	
}








