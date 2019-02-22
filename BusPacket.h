/*********************************************************************************
*  Copyright (c) 2018-2019, 
*  Reza Mirosanlou
*  University of Waterloo               
*  All rights reserved.
*********************************************************************************/

#ifndef BUSPACKET_H
#define BUSPACKET_H

#include <cstdint>

namespace DRAMController
{
	enum BusPacketType
	{
		RD, RDA, WR, WRA,		// CAS access
		ACT_R, ACT_W, PRE, PREA,			// Open/Close operation
		REF,					// Refresh
		PDE, PDX, SRE, SRX,		// Power Related Command
		DATA
	};

	class BusPacket
	{
	public:
		BusPacket(BusPacketType packetType, uint64_t id, uint64_t addr, 
			 unsigned col, unsigned rw, unsigned bank, unsigned rank, void *data, unsigned time):
		busPacketType(packetType),
		requestorID(id),
		address(addr),
		column(col),
		row(rw),
		bank(bank),
		rank(rank),
		data(data),
		arriveTime(time)
		{
			postCommand = false;
			postCounter = 0;
		}

		BusPacketType busPacketType;
		uint64_t requestorID;
		uint64_t address;
		unsigned column;
		unsigned row;
		unsigned subArray;
		unsigned bank;
		unsigned bankGroup;
		unsigned rank;
		void *data;
		unsigned arriveTime;

		bool postCommand;
		unsigned postCounter;
		unsigned int addressMap[4];
	};
}
#endif

