

#include "../../CommandScheduler.h"

namespace DRAMController
{
	class CommandScheduler_ReOrder: public CommandScheduler
	{
	private:
		vector<std::pair<BusPacket*, unsigned int>> commandRegisters;
		unsigned int indexCAS;
		unsigned int indexACT;
		unsigned int indexPRE;

		vector<bool> servedFlags;
		vector<bool> queuePending;
		unsigned int bundlingType;

		vector<pair<BusPacket*, unsigned int>> srtFIFO;


		// Virtual Channel
		unsigned virChannel; 
		std::map<unsigned int, unsigned int> virMap;
		vector<bool> virChannelRegister; 

	public:
		CommandScheduler_ReOrder(vector<CommandQueue*>& commandQueues, const map<unsigned int, bool>& requestorTable):
			CommandScheduler(commandQueues, requestorTable)
		{
			bundlingType = BusPacketType::RD;	// Read Type
			indexCAS = 0;
			indexACT = 0;
			indexPRE = 0;

			// Virtual Channel
			virChannel = 0;
			if(virChannel>0){
				for(unsigned index=0; index<virChannel; index++) {
					virChannelRegister.push_back(false);
				}				
			}

			for(unsigned index=0; index < commandQueue.size(); index++) {
				queuePending.push_back(false);
			}
			for(unsigned index=0; index < commandQueue.size()+virChannel; index++) {
				servedFlags.push_back(false);
			}
		}

		BusPacket* commandSchedule()
		{
			scheduledCommand = NULL;
			// Scan the command queue for ready CAS
			for(unsigned int index = 0; index < commandQueue.size(); index++) {
				checkCommand = NULL;
				if(commandQueue[index]->getSize(true) > 0 && queuePending[index] == false) {
					checkCommand = commandQueue[index]->getCommand(true);
				}
				else if(commandQueue[index]->getSize(false) > 0 && queuePending[index] == false) {
					checkCommand = commandQueue[index]->getCommand(false);
				}
				if(checkCommand!=NULL) {
					if(isReady(checkCommand, index)) {
						if(requestorCriticalTable.at(checkCommand->requestorID) == true) {
							commandRegisters.push_back(make_pair(checkCommand,index));
						}
						else {
							srtFIFO.push_back(make_pair(checkCommand,index));
						}
						queuePending[index] = true;	
					}
				}
			}

			for(unsigned int index=0; index<virChannelRegister.size(); index++) {
				if(virChannelRegister[index]==false && !srtFIFO.empty()) {
					commandRegisters.push_back(srtFIFO.front());
					virMap[srtFIFO.front().first->requestorID] = commandQueue.size() + index;
					virChannelRegister[index] = true;
					srtFIFO.erase(srtFIFO.begin());
				}
			}

			bool newRound = true;	// should reset servedFlag?
			bool newType = true;	// should switch casType?
			bool newCAS = false;	// a different CAS can be schedule
			bool haveCAS = false;
			// Served Flag and access type Checking
			if(!commandRegisters.empty()) {
				for(unsigned int index =0; index < commandRegisters.size(); index++){
					checkCommand = commandRegisters[index].first;
					if(checkCommand->busPacketType < ACT) {
						haveCAS = true;

						bool isServed = true;
						// virtual Channel
						if(requestorCriticalTable.at(checkCommand->requestorID) == false) {
							isServed = servedFlags[virMap[checkCommand->requestorID]];
						}
						else {
							isServed = servedFlags[commandRegisters[index].second];
						}
						if(isServed == false) {
							newRound = false;
							if(checkCommand->busPacketType == bundlingType) {
								newType = false;
								break;
							}
							else {
								newCAS = true;
							}							
						}
					}
				}
				// Reset service bits
				if(newRound) {
					for(unsigned int index = 0; index < servedFlags.size(); index++) {
						servedFlags[index] = false;
					}
				}
				if(haveCAS) {
					// Switch access type
					if(newType && newCAS) {
						if(bundlingType == WR) {bundlingType = RD;}
						else {bundlingType = WR;}
					}					
				}
				checkCommand = NULL;	
			}
			// Schedule the FIFO with CAS blocking
			unsigned int registerIndex = 0;
			if(!commandRegisters.empty()) {
				for(unsigned int index = 0; index < commandRegisters.size(); index++) {
					checkCommand = commandRegisters[index].first;

					// Virtual Channel
					if(requestorCriticalTable.at(checkCommand->requestorID) == false) {
						if(servedFlags[virMap[checkCommand->requestorID]] == false && checkCommand->busPacketType == bundlingType) {
							if(isIssuable(checkCommand)) {
								scheduledCommand = checkCommand;
								registerIndex = index;
								break;
							}							
						}
					}
					else {
						// Not served and same CAS type
						if(servedFlags[checkCommand->bank] == false && checkCommand->busPacketType == bundlingType) {
							if(isIssuable(checkCommand)) {
								scheduledCommand = checkCommand;
								registerIndex = index;
								break;
							}
						}						
					}
					checkCommand = NULL;
				}				
			}

			// Round-Robin ACT arbitration
			if(scheduledCommand == NULL && !commandRegisters.empty()) {
				for(unsigned int index = 0; index < commandRegisters.size(); index++) {
					if(commandRegisters[index].first->busPacketType == ACT) {
						checkCommand = commandRegisters[index].first;
						if(isIssuable(checkCommand)) {
							scheduledCommand = checkCommand;
							registerIndex = index;
							break;
						}
						checkCommand = NULL;
					}
				}
			}
			// Round-Robin PRE arbitration
			if(scheduledCommand == NULL && !commandRegisters.empty()) {
				for(unsigned int index = 0; index < commandRegisters.size(); index++) {
					if(commandRegisters[index].first->busPacketType == PRE) {
						checkCommand = commandRegisters[index].first;
						if(isIssuable(checkCommand)) {
							scheduledCommand = checkCommand;
							registerIndex = index;
							break;
						}
					}
				}
			}

			if(scheduledCommand != NULL) {
				if(scheduledCommand->busPacketType < ACT) {
					if(requestorCriticalTable.at(scheduledCommand->requestorID) == false) {
						servedFlags[virMap[scheduledCommand->requestorID]] = true;
					}
					else {
						servedFlags[scheduledCommand->bank] = true;
					}
				}
				queuePending[commandRegisters[registerIndex].second] = false;
				sendCommand(scheduledCommand, commandRegisters[registerIndex].second);
				// Virtual Channel
				if(requestorCriticalTable.at(scheduledCommand->requestorID) == false) {
					virChannelRegister[virMap[scheduledCommand->requestorID]-commandQueue.size()] = false;
				}
				commandRegisters.erase(commandRegisters.begin()+registerIndex);
			}
			else {
				if(!srtFIFO.empty() && commandRegisters.empty()) {
					for(unsigned int index=0; index<srtFIFO.size(); index++) {
						if(isIssuable(srtFIFO[index].first)) {
							scheduledCommand = srtFIFO[index].first;
							sendCommand(scheduledCommand, srtFIFO[index].second);
							queuePending[srtFIFO[index].second] = false;
							srtFIFO.erase(srtFIFO.begin() + index);
							break;
						}
					}
				}
			}
		
			checkCommand = NULL;	
			return scheduledCommand;
		}
	};
}