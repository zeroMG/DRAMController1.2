/*********************************************************************************
*  Copyright (c) 2015-2016, Danlu Guo
*                             University of Waterloo
*                
*  All rights reserved.
*********************************************************************************/

#ifndef REQUESTQUEUE_H
#define REQUESTQUEUE_H

#include <vector>
#include <map>

#include "Request.h"

namespace DRAMController
{
	class RequestQueue
	{
	public:
		RequestQueue(bool perRequestor, bool writeQueueEable);
		virtual ~RequestQueue();
		bool isWriteEnable();
		bool isPerRequestor();

		bool insertRequest(Request* request);
		// get the number of requestor buffer 
		unsigned int getQueueSize();		
		// get the number of requests in either the individual buffer, or general buffer
		// 		the index for requestor is the index in the requestorOrder vector, not ID
		unsigned int getSize(bool requestor, unsigned int index);		
		// get request from individual buffer
		//		reqInde also indicates the index in requestorOrder vector
		//		index then indicates the index in the corresponding requestorBuffer	
		Request* getRequest(unsigned int reqIndex, unsigned int index);	
		// get request from general buffer																
		Request* getRequest(unsigned int index);
		// remove the most recently accessed request
		void removeRequest();

		class WriteQueue
		{
		public:
			WriteQueue(unsigned int low, unsigned int high){
				lowwatermark = low;
				highwatermark = high;
			}
			virtual ~WriteQueue() {
				for(auto it=writeQueue.begin(); it!=writeQueue.end(); it++) { delete (*it); }
				writeQueue.clear();
			}
			void insertWrite(Request* request){ writeQueue.push_back(request); }
			unsigned int bufferSize() {return writeQueue.size(); }
			bool highWatermark() {
				if(writeQueue.size() >= highwatermark) {return true;}
				else{ return false; }
			}
			bool lowWatermark() {
				if(writeQueue.size() <= lowwatermark) { return true; }
				else {return false; }
			}
			Request* getWrite(unsigned int index){ return writeQueue[index]; }
			void removeWrite(unsigned int index){ writeQueue.erase(writeQueue.begin()+index); }

		private:
			unsigned int lowwatermark;
			unsigned int highwatermark;
			std::vector<Request*> writeQueue;
		};
		WriteQueue* writeQueue;

	private:
		bool writeQueueEnable;
		bool perRequestorEnable;

		// unsigned int totalRequest;
		std::vector<Request*> generalBuffer;
		std::map<unsigned int, std::vector<Request*> > requestorBuffer;
		// record the requestor target to the request queue
		// order matters for intra-bank scheduling among all requestors
		std::vector<unsigned int> requestorOrder;

		// bool indicate as requestor or general buffer
		std::pair<bool, std::pair<unsigned int, unsigned int>> scheduledRequest;		
	};
}
#endif




