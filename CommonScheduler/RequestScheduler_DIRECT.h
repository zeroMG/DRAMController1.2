#ifndef REQUESTSCHEDULER_DIRECT_H
#define REQUESTSCHEDULER_DIRECT_H

#include "../RequestScheduler.h"

namespace DRAMController
{
	class RequestScheduler_Direct: public RequestScheduler
	{
	private:
		bool roundRobin_Level = false;
		vector<unsigned int> requestorIndex;

	public:
		RequestScheduler_Direct(std::vector<RequestQueue*>&requestQueues, std::vector<CommandQueue*>& commandQueues, const std::map<unsigned int, bool>& requestorTable): 
			RequestScheduler(requestQueues, commandQueues, requestorTable) 
		{
			for(unsigned int index = 0; index < requestQueue.size(); index++) {
				requestorIndex.push_back(0);
			}
		}

		void requestSchedule()
		{
			for(size_t index =0; index < requestQueue.size(); index++) {
				// Requestor Direct Connection per Memory Level
				if(requestQueue[index]->isPerRequestor())
				{
					if(requestQueue[index]->getQueueSize() > 0)
					{
						for(unsigned int num=0; num<requestQueue[index]->getQueueSize(); num++)
						{
							scheduledRequest = NULL;
							// requestorIndex[index]=num;
							if(requestQueue[index]->getSize(true, requestorIndex[index]) > 0) 
							{
								scheduledRequest = requestQueue[index]->getRequest(requestorIndex[index], 0);
								// DEBUG("THERE IS REQUESTS");
								if(isSchedulable(scheduledRequest, isRowHit(scheduledRequest)))
								{
									updateRowTable(scheduledRequest->addressMap[Rank], scheduledRequest->addressMap[Bank], scheduledRequest->row);
									requestQueue[index]->removeRequest();
								}
							}
							requestorIndex[index]++;
							if(requestorIndex[index] == requestQueue[index]->getQueueSize()) {
								requestorIndex[index]=0;
							}
							if(roundRobin_Level == true && scheduledRequest!=NULL)
							{
								break;
							}
						}
					}
					scheduledRequest = NULL;
				}
				// Direct Connectino of Memory Level
				else
				{
					if(requestQueue[index]->getSize(false,0) > 0)
					{
						// scheduledRequest = requestQueue[index]->getRequest(0);
						scheduledRequest = scheduleFR(index);
						if(isSchedulable(scheduledRequest, isRowHit(scheduledRequest)))
						{
							updateRowTable(scheduledRequest->rank, scheduledRequest->bank, scheduledRequest->row);
							requestQueue[index]->removeRequest();
						}
						scheduledRequest = NULL;
					}
				}					
			}
		}
	};
}

#endif

