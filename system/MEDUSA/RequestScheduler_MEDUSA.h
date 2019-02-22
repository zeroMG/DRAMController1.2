#ifndef REQUESTSCHEDULER_MEDUSA_H
#define REQUESTSCHEDULER_MEDUSA_H

#include "../../RequestScheduler.h"

namespace DRAMController
{
	class RequestScheduler_MEDUSA: public RequestScheduler
	{
	private:
		bool writeEnable;

	public:
		RequestScheduler_MEDUSA(std::vector<RequestQueue*>&requestQueues, const std::map<unsigned int, bool>& requestorTable): 
			RequestScheduler(requestQueues, requestorTable) 
		{
			DEBUG("	RequestScheduler : MEDUSA");
		}

		void requestSchedule()
		{
			Request* tempRequest = NULL;
			RequestQueue* tempQueue = NULL;

			// Read Request Checking
			writeEnable = true;
			for(size_t index =0; index < requestQueue.size(); index++)
			{
				tempQueue = requestQueue[index];
				tempRequest = tempQueue->getRequest(0,0);	
				if(tempRequest != NULL) {
					writeEnable = false;
					if(isSchedulable(tempRequest, isRowHit(tempRequest))) {
						tempQueue->removeRequest();
					}
				}
			}
			// Write Request Checking
			if(writeEnable) {
				RequestQueue::WriteQueue* writeQ = NULL;
				for(int index=0; index < requestQueue.size(); index++) {
					writeQ = requestQueue[index]->writeQueue;
					if( writeQ != NULL) {
						if(writeQ->highWatermark()) {
							for(int qIndex=0; qIndex < writeQ->bufferSize(); qIndex++) {
								tempRequest = writeQ->getWrite(qIndex);
								if(isRowHit(tempRequest)) {
									if(isSchedulable(tempRequest, true)) {
										writeQ->popWrite(qIndex);
									}
								}
							}
						}
						int qIndex = 0;
						while(!writeQ->lowWatermark()) {
							tempRequest = writeQ->getWrite(qIndex);
							if(isSchedulable(tempRequest, isRowHit(tempRequest))) {
								writeQ->popWrite(qIndex);
							}
							qIndex++;
						}
					}
				}
			}
			tempQueue = NULL;
			tempRequest = NULL;
			delete(tempQueue);
			delete(tempRequest);
		}
	};
}
#endif