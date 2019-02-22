
#ifndef COMMANDGENERATOR_PIPECAS_H
#define COMMANDGENERATOR_PIPECAS_H

#include "../../CommandGenerator.h"

namespace DRAMController
{
	class CommandGenerator_PipeCAS: public CommandGenerator
	{
	private:
		const std::map<unsigned int, bool>& requestorCriticalTable;
	public:
		CommandGenerator_PipeCAS(unsigned int dataBus, const std::map<unsigned int, bool>& requestorTable):
			CommandGenerator(dataBus),
			requestorCriticalTable(requestorTable)
		{
			// requestorCriticalTable = requestorTable;
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
			if(requestorCriticalTable.at(request->requestorID) == false) {
				CAS = RD;
			}
			if(request->requestType == DATA_WRITE) {
				CAS = WRA;
				if(requestorCriticalTable.at(request->requestorID) == false) {
					CAS = WR;
				}
			}
			unsigned id = request->requestorID;
			unsigned address = request->address;
			unsigned rank = request->rank;
			unsigned row = request->row;
			unsigned col = request->col;

			unsigned bank = request->bank;

			if(requestorCriticalTable.at(request->requestorID)) {
				commandBuffer.push(new BusPacket(ACT, id, address, col, row, bank, rank, 0, 0));
				commandBuffer.push(new BusPacket(CAS, id, address, col, row, bank, rank, request->data, 0));
			}
			else {
				if(!open) {
					commandBuffer.push(new BusPacket(PRE, id, address, col, row, bank, rank, 0, 0));
					commandBuffer.push(new BusPacket(ACT, id, address, col, row, bank, rank, 0, 0));
				}
				commandBuffer.push(new BusPacket(CAS, id, address, col, row, bank, rank, request->data, 0));
			}

			// for(unsigned int bi = 0; bi < lookupTable[request->requestSize].first; bi++)
			// {
			// 	unsigned bank = (request->bank + bi)%16;
			// 	for(unsigned int bc = 0; bc < lookupTable[request->requestSize].second; bc++)
			// 	{
			// 		commandBuffer.push(new BusPacket(ACT, id, address, col, row, bank, rank, 0, clockCycle));
			// 		commandBuffer.push(new BusPacket(CAS, id, address, col, row, bank, rank, request->data, clockCycle));
			// 	}
			// }
			return true;
		}
	};
}

#endif