/*********************************************************************************
*  Copyright (c) 2018-2019, 
*  Reza Mirosanlou
*  University of Waterloo               
*  All rights reserved.
*********************************************************************************/

#include "BusPacket.h"
#include "CommandQueue.h"

#include <vector>

using namespace std;

namespace DRAMController
{
	class RefreshMachine
	{
	public:
		RefreshMachine(const vector<CommandQueue*>& commandQueues, size_t ranks, size_t banks, size_t tREFI, size_t tRFC):
			commandQueue(commandQueues),
			ranks(ranks),
			// banks(banks),
			tREFI(tREFI),
			tRFC(tRFC)
		{
			globeRefreshCountdown = tREFI;
			refreshcount = 0;
			refreshCounting = 0;
			isRefreshing = false;
		}
		virtual ~RefreshMachine()
		{
			commandBuffer.clear();
		}

		bool refreshing()
		{
			return isRefreshing;
		}
		void refresh(BusPacket* &refCmd)
		{
			if(!commandBuffer.empty()) {
				refCmd = commandBuffer.front();
			}
			else {
				if(refreshCounting == 0) {
					isRefreshing = false;
				}
			}
		}
		void popCommand()
		{
			if(commandBuffer.front()->busPacketType == REF) {
				refreshCounting = tRFC;
			}
			BusPacket* tempCmd = commandBuffer.front();
			commandBuffer.erase(commandBuffer.begin());
			delete tempCmd;
		}
		void commandGenerate()
		{
			for(size_t rank=0; rank<ranks; rank++) {
				commandBuffer.push_back(new BusPacket(PREA, 0, 0, 0, 0, rank, 0, NULL, 0));
				commandBuffer.push_back(new BusPacket(REF, 0, 0, 0, 0, rank, 0, NULL, 0));
			}
			for(unsigned index=0; index < commandQueue.size(); index++) {
				BusPacket* tempCmd = commandQueue[index]->getCommand(true);
				if( tempCmd == NULL) {
					tempCmd = commandQueue[index]->getCommand(false);
				}
				if(tempCmd!=NULL) {
					// If there is pending CAS commands, the refresh must reopen the close banks to remain the same condition
					if(tempCmd->busPacketType < WR) {
						commandBuffer.push_back(new BusPacket(ACT_R, 0, 0, 0, tempCmd->row, tempCmd->bank, tempCmd->rank, NULL, 0));
					}
					else if(tempCmd->busPacketType < ACT_R) {
						commandBuffer.push_back(new BusPacket(ACT_W, 0, 0, 0, tempCmd->row, tempCmd->bank, tempCmd->rank, NULL, 0));
					}
				}
			}
		}

		void step()
		{
			//globeRefreshCountdown--;
			if(globeRefreshCountdown == 0) {
				refreshcount++;
				cout<<"Refresh Count***************************************************************************************************************************************  "<<refreshcount<<endl;
				cout<<"Refresh Count***************************************************************************************************************************************  "<<refreshcount<<endl;
				cout<<"Refresh Count***************************************************************************************************************************************  "<<refreshcount<<endl;
				cout<<"Refresh Count***************************************************************************************************************************************  "<<refreshcount<<endl;
				isRefreshing = true;
				globeRefreshCountdown = tREFI;
				// generate refresh commands 
				commandGenerate();
			}
			if(refreshCounting != 0) {
				refreshCounting--;
			}	
		}

	private:
		// connect to commandQueue so that compensate commands can be generated
		const vector<CommandQueue*>& commandQueue;
		std::vector<BusPacket*> commandBuffer;

		size_t ranks;
		// size_t banks;
		size_t tREFI;
		size_t tRFC;
		bool isRefreshing;
		unsigned globeRefreshCountdown;
		unsigned refreshCounting;
		unsigned int refreshcount;
	};
}
