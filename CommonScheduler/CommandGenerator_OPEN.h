
#ifndef COMMANDGENERATOR_OPEN_H
#define COMMANDGENERATOR_OPEN_H

#include "../CommandGenerator.h"

namespace DRAMController
{
	class CommandGenerator_Open: public CommandGenerator
	{
	public:
		CommandGenerator_Open(unsigned int dataBus):CommandGenerator(dataBus)
		{}

		// The command generator should know which command queue to check for schedulability
		bool commandGenerate(Request* request, bool open)
		{
			int actdefine = 0;
			BusPacketType CAS = RD;
			if(request->requestType == DATA_WRITE) {
				CAS = WR;
				actdefine = 1;
			}
			unsigned size = request->requestSize/dataBusSize; 
			unsigned id = request->requestorID;
			unsigned address = request->address;
			unsigned rank = request->rank;
			unsigned bank = request->bank;
			unsigned row = request->row;
			unsigned col = request->col;

			if(!open) {
				if (actdefine == 0){
				commandBuffer.push(new BusPacket(PRE, id, address, 0, row, bank, rank, NULL, 0));
				commandBuffer.push(new BusPacket(ACT_R, id, address, 0, row, bank, rank, NULL, 0));	
				}
				else{
				commandBuffer.push(new BusPacket(PRE, id, address, 0, row, bank, rank, NULL, 0));
				commandBuffer.push(new BusPacket(ACT_W, id, address, 0, row, bank, rank, NULL, 0));		
				}
			}
			for(unsigned int x = 0; x < size; x++) {
				commandBuffer.push(new BusPacket(CAS, id, address, col+size, row, bank, rank, request->data, 0));
			}
			return true;
		}
	};
}
#endif
