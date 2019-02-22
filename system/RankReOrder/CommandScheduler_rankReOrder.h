

#include "../../CommandScheduler.h"

namespace DRAMController
{
	class CommandScheduler_rankReOrder: public CommandScheduler
	{
	private:
		unsigned int bundlingType;
		unsigned int rankIndex;

		vector< vector<BusPacket*> > CASBuffer;
		vector< vector<unsigned int>> CASQueueIndex;
		vector<BusPacket*> ACT_PREBuffer;
		vector<unsigned int> CMDQueueIndex;

		vector<bool> queuePending;
		vector<bool> servedFlag;

		unsigned int rank_num;

	public:
		CommandScheduler_rankReOrder(vector<CommandQueue*>& commandQueues, const map<unsigned int, bool>& requestorTable):
			CommandScheduler(commandQueues, requestorTable)
		{
			rankIndex = 0;
			rank_num = 4;
			bundlingType = BusPacketType::RD;	// Read Type
			for(unsigned int index = 0; index < commandQueue.size(); index++) {
				queuePending.push_back(false);
				servedFlag.push_back(false);
			}
			for(unsigned int rank = 0; rank < rank_num; rank++) {
				CASBuffer.push_back(vector<BusPacket*>());
				CASQueueIndex.push_back(vector<unsigned int>());
			}
		}

		BusPacket* commandSchedule()
		{
			scheduledCommand = NULL;
			checkCommand = NULL;
			// Check if all the CAS commands have been issued
			bool newRound = true;
			bool newType = true;
			for(unsigned index = 0; index < CASBuffer.size(); index++) {
				if(!CASBuffer[index].empty()) {
					newRound = false;
					for(unsigned req = 0; req < CASBuffer[index].size(); req++) {
						if(CASBuffer[index][req]->busPacketType == bundlingType) {
							newType = false;
						}
					}
				}
			}			
			if(newRound) {
				for(unsigned int index=0; index<servedFlag.size(); index++) {
					servedFlag[index] = false;
				}
			}
			// Switch Type
			if(!newRound && newType) {
				if(bundlingType == BusPacketType::RD) {bundlingType = BusPacketType::WR;}
				else {bundlingType = BusPacketType::RD;}
			}			

			// Scan the command queue for ready CAS
			for(unsigned int index = 0; index < commandQueue.size(); index++) {
				if(queuePending[index] == false) {
					if(commandQueue[index]->getSize(true) > 0) {
						checkCommand = commandQueue[index]->getCommand(true);
					}
					// SRTs
					if(checkCommand==NULL && commandQueue[index]->getSize(false)>0) {
						checkCommand = commandQueue[index]->getCommand(false);
					}
				}
				if(checkCommand!=NULL) {
					if(isReady(checkCommand, index)) {
						if(checkCommand->busPacketType >= ACT) {
							queuePending[index] = true;
							ACT_PREBuffer.push_back(checkCommand);
							CMDQueueIndex.push_back(index);
						}
						else {
							if(servedFlag[index] == false) {
								queuePending[index] = true;
								// DEBUG(clock<<" CMD"<<checkCommand->busPacketType<<" R"<<checkCommand->rank<<" B"<<checkCommand->bank<<" REQ"<<checkCommand->requestorID);
								CASBuffer[checkCommand->rank].push_back(checkCommand);
								CASQueueIndex[checkCommand->rank].push_back(index);
							}
						}
					}
					checkCommand = NULL;
				}
			}
			
			unsigned schduledIndex = 0;
			checkCommand = NULL;
			for(unsigned int rank = 0; rank < rank_num; rank++) 
			{
				if(!CASBuffer[rankIndex].empty()) 
				{
					for(unsigned int index=0; index<CASBuffer[rankIndex].size(); index++) {
						checkCommand = CASBuffer[rankIndex][index];
						if(checkCommand->busPacketType == bundlingType) {
							if(isIssuable(checkCommand)) {
								scheduledCommand = checkCommand;
								schduledIndex = CASQueueIndex[rankIndex][index];
								CASBuffer[rankIndex].erase(CASBuffer[rankIndex].begin() + index);
								CASQueueIndex[rankIndex].erase(CASQueueIndex[rankIndex].begin() + index);
							}
							break;
						}
					}
				}
				if(scheduledCommand != NULL) {
					break;
				}
				rankIndex++;
				if(rankIndex == rank_num) {
					rankIndex = 0;
				}
			}
			
						
			if(scheduledCommand == NULL && !ACT_PREBuffer.empty())
			{
				for(unsigned int index=0; index<ACT_PREBuffer.size(); index++) {
					if(isIssuable(ACT_PREBuffer[index])) {
						scheduledCommand = ACT_PREBuffer[index];
						schduledIndex = CMDQueueIndex[index];
						ACT_PREBuffer.erase(ACT_PREBuffer.begin() + index);
						CMDQueueIndex.erase(CMDQueueIndex.begin() + index);
						break;
					}
				}
			}		


			if(scheduledCommand != NULL) {
				if(scheduledCommand->busPacketType < ACT) {
					servedFlag[schduledIndex] = true;
				}
				queuePending[schduledIndex] = false;
				// DEBUG("SEND OUT FROM "<<schduledIndex);
				sendCommand(scheduledCommand, schduledIndex);
			}
			return scheduledCommand;
		}
	};
}