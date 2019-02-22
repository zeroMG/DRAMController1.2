
#ifndef COMMANDGENERATOR_CLOSE_H
#define COMMANDGENERATOR_CLOSE_H

#include "../CommandGenerator.h"

namespace DRAMController
{
	class CommandGenerator_Close: public CommandGenerator
	{
	public:
		CommandGenerator_Close(unsigned int dataBus):
			CommandGenerator(dataBus)
		{
			lookupTable[dataBus] = make_pair(1,1);
			lookupTable[dataBus*2] = make_pair(2,1);
			lookupTable[dataBus*4] = make_pair(4,1);
			lookupTable[dataBus*8] = make_pair(8,1);
			lookupTable[dataBus*16] = make_pair(8,2);	
		}

		// The command generator should know which command queue to check for schedulability
		bool commandGenerate(Request* request, bool open)
		{
			BusPacketType CAS = RDA;
			if(request->requestType == DATA_WRITE) {
				CAS = WRA;
			}
			unsigned id = request->requestorID;
			unsigned address = request->address;
			unsigned rank = request->rank;
			unsigned row = request->row;
			unsigned col = request->col;

			for(unsigned int bi = 0; bi < lookupTable[request->requestSize].first; bi++)
			{
				unsigned bank = (request->bank + bi)%16;
				for(unsigned int bc = 0; bc < lookupTable[request->requestSize].second; bc++)
				{
					commandBuffer.push(new BusPacket(ACT, id, address, col, row, bank, rank, 0, 0));
					commandBuffer.push(new BusPacket(CAS, id, address, col, row, bank, rank, request->data, 0));
				}
			}
			return true;
		}
	};
}

#endif