
#ifndef COMMANDSCHEDULER_ORP_H
#define COMMANDSCHEDULER_ORP_H

#include "../../CommandScheduler.h"

namespace DRAMController
{
	class CommandScheduler_ORP: public CommandScheduler
	{
	private:
		vector<pair<BusPacket*, unsigned int>> cmdFIFO;		// FIFO contains ready commands and queue index
		vector<pair<BusPacket*, unsigned int>> srtFIFO;
		// Pending Command indicator based on requestorID
		map<unsigned int, bool> queuePending;
		

	public:
		CommandScheduler_ORP(vector<CommandQueue*>& commandQueues, const map<unsigned int, bool>& requestorTable):
			CommandScheduler(commandQueues, requestorTable)
		{}

		~CommandScheduler_ORP()
		{
			for(auto it=cmdFIFO.begin(); it!=cmdFIFO.end(); it++) {
				delete it->first;
			}
			cmdFIFO.clear();
			queuePending.clear();
		}

		BusPacket* commandSchedule()
		{
			checkCommand = NULL;
			for(unsigned int index = 0; index < commandQueue.size(); index++)
			{
				// PerRequestor is enabled and there is some requestors in this memory level
				if(commandQueue[index]->isPerRequestor()) 
				{
					if(commandQueue[index]->getRequestorIndex() > 0) // there is more than 0 requestor in the design
					{
						for(unsigned int num=0; num<commandQueue[index]->getRequestorIndex(); num++) // for all requestors from "num"
						{
							if(commandQueue[index]->getRequestorSize(num) > 0 ) //return the buffer size of the requestor
							{
								checkCommand = commandQueue[index]->getRequestorCommand(num);
								if(queuePending.find(checkCommand->requestorID) == queuePending.end()) {
									queuePending[checkCommand->requestorID] = false;
								}

								if(queuePending[checkCommand->requestorID] == false && isReady(checkCommand, index)) {
									if(requestorCriticalTable.at(checkCommand->requestorID) == true) {
										cmdFIFO.push_back(std::make_pair(checkCommand,index));
									}
									else {
										// Equally
										cmdFIFO.push_back(std::make_pair(checkCommand,index));
										// Fixed Priority
										// srtFIFO.push_back(std::make_pair(checkCommand,index));
									}
									
									queuePending[checkCommand->requestorID] = true;
								}
								checkCommand = NULL;
							}
						}
					}	
				}
				// single memory level queue with hrt and srt
				else
				{
					if(commandQueue[index]->getSize(true) > 0 && queuePending[index] == false)
					{
						checkCommand = commandQueue[index]->getCommand(true);
					}
					if(checkCommand == NULL && commandQueue[index]->getSize(false) > 0)
					{
						checkCommand = commandQueue[index]->getCommand(false);
					}
					if(checkCommand != NULL) {
						if(isReady(checkCommand, index)) {
							cmdFIFO.push_back(std::make_pair(checkCommand,index));
							queuePending[index] = true;
						}
						checkCommand = NULL;
					}
				}
			}

			scheduledCommand = NULL;
			// Schedule the FIFO with CAS blocking
			bool casBlocking = false;
			if(cmdFIFO.size() > 0) {
				for(unsigned int index = 0; index < cmdFIFO.size(); index++) {
					checkCommand = cmdFIFO[index].first;
					if(checkCommand->busPacketType < ACT && casBlocking) {
						continue;
					}
					if(isIssuable(checkCommand)) {
						scheduledCommand = checkCommand;
						sendCommand(scheduledCommand, cmdFIFO[index].second);
						if(commandQueue[cmdFIFO[index].second]->isPerRequestor()) {
							queuePending[scheduledCommand->requestorID] = false;
						}
						else {
							queuePending[cmdFIFO[index].second] = false;
						}
						cmdFIFO.erase(cmdFIFO.begin() + index);
						return scheduledCommand;
					}
					else{
						if(checkCommand->busPacketType < ACT) {
							casBlocking = true;
						}
					}
				}
			}

			if(srtFIFO.size()>0 && !casBlocking) {
				for(unsigned int index=0; index < srtFIFO.size(); index++) {
					if(isIssuable(srtFIFO[index].first)) {
						scheduledCommand = srtFIFO[index].first;
						sendCommand(scheduledCommand, srtFIFO[index].second);
						queuePending[scheduledCommand->requestorID] = false;
						srtFIFO.erase(srtFIFO.begin() + index);
						return scheduledCommand;
					}
				}
			}

			return NULL;
		}
	};
}
#endif
