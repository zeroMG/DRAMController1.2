
#ifndef COMMANDGENERATOR_MEDUSA_H
#define COMMANDGENERATOR_MEDUSA_H

#include "../../CommandGenerator.h"

namespace DRAMController
{
	class CommandGenerator_MEDUSA: public CommandGenerator
	{
	public:
		CommandGenerator_MEDUSA(unsigned int dataBus, std::vector<CommandQueue*>& commandQueues):
			CommandGenerator(dataBus, commandQueues)
		{
			DEBUG("	CommandGenerator : MEDUSA");		
		}

		// The command generator should know which command queue to check for schedulability
		bool isSchedulable(Request* request, bool open)
		{
			commandGenerate(request, open);	
			return true;
		}

		void commandGenerate(Request* request, bool open)
		{
			BusPacketType CAS = RD;
			if(request->requestType == DATA_WRITE) {
				CAS = WR;
			}
			unsigned size = request->requestSize/dataBusSize; 
			unsigned id = request->requestorID;
			unsigned address = request->address;
			unsigned rank = request->rank;
			unsigned bank = request->bank;
			unsigned row = request->row;
			unsigned col = request->col;

			if(!open) {
				commandBuffer.push(new BusPacket(PRE, id, address, 0, 0, bank, rank, NULL, clockCycle));
				commandBuffer.push(new BusPacket(ACT, id, address, col, row, bank, rank, request->data, clockCycle));
			}
			for(unsigned int x = 0; x < size; x++) {
				commandBuffer.push(new BusPacket(CAS, id, address, col+x, row, bank, rank, request->data, clockCycle));
			}
		}
	};
}
#endif
